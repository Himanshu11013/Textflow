"""
Text prediction service using simple n-gram models and neural networks.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import numpy as np
from collections import defaultdict, Counter
import re
import random

logger = logging.getLogger(__name__)

class PredictionService:
    def __init__(self):
        self.ngram_model = None
        self.vocabulary = set()
        self.initialized = False
    
    async def load_models(self):
        """Load prediction models."""
        try:
            # Initialize n-gram model
            self.ngram_model = defaultdict(Counter)
            self.vocabulary = set()
            
            self.initialized = True
            logger.info("Prediction service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize prediction service: {e}")
            raise
    
    async def predict_next_words(self, text: str, max_predictions: int = 5, 
                               context_length: int = 50) -> Dict[str, Any]:
        """Predict next words based on context."""
        if not self.initialized:
            await self.load_models()
        
        try:
            # Clean and tokenize text
            words = self._tokenize(text)
            
            if len(words) < 2:
                return {
                    "predictions": [],
                    "probabilities": [],
                    "context": text
                }
            
            # Get context (last few words)
            context = words[-min(context_length, len(words)):]
            
            # Generate predictions
            predictions = self._generate_predictions(context, max_predictions)
            
            return {
                "predictions": [pred["word"] for pred in predictions],
                "probabilities": [pred["probability"] for pred in predictions],
                "context": " ".join(context)
            }
            
        except Exception as e:
            logger.error(f"Text prediction error: {e}")
            return {
                "predictions": [],
                "probabilities": [],
                "context": text
            }
    
    async def complete_text(self, text: str) -> str:
        """Complete incomplete text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            words = self._tokenize(text)
            
            if not words:
                return text
            
            # Get the last word (might be incomplete)
            last_word = words[-1]
            
            # Find completions for the last word
            completions = self._find_completions(last_word)
            
            if completions:
                # Replace the last word with the best completion
                words[-1] = completions[0]["word"]
                return " ".join(words)
            else:
                # Try to predict next words
                predictions = await self.predict_next_words(text, max_predictions=1)
                if predictions["predictions"]:
                    return text + " " + predictions["predictions"][0]
            
            return text
            
        except Exception as e:
            logger.error(f"Text completion error: {e}")
            return text
    
    async def train_model(self, training_texts: List[str]) -> Dict[str, Any]:
        """Train the prediction model on provided texts."""
        if not self.initialized:
            await self.load_models()
        
        try:
            total_words = 0
            ngram_counts = defaultdict(Counter)
            
            for text in training_texts:
                words = self._tokenize(text)
                total_words += len(words)
                
                # Build n-gram model (trigrams)
                for i in range(len(words) - 2):
                    context = " ".join(words[i:i+2])
                    next_word = words[i+2]
                    ngram_counts[context][next_word] += 1
                
                # Add to vocabulary
                self.vocabulary.update(words)
            
            # Update the model
            self.ngram_model = ngram_counts
            
            return {
                "status": "success",
                "total_words": total_words,
                "vocabulary_size": len(self.vocabulary),
                "ngram_count": len(ngram_counts)
            }
            
        except Exception as e:
            logger.error(f"Model training error: {e}")
            return {
                "status": "error",
                "message": str(e)
            }
    
    async def get_suggestions(self, text: str, position: int) -> List[str]:
        """Get word suggestions at a specific position."""
        if not self.initialized:
            await self.load_models()
        
        try:
            words = self._tokenize(text)
            
            if position >= len(words):
                # At the end of text, predict next words
                predictions = await self.predict_next_words(text, max_predictions=5)
                return predictions["predictions"]
            
            # Get context around the position
            start = max(0, position - 10)
            end = min(len(words), position + 10)
            context = words[start:end]
            
            # Generate suggestions based on context
            suggestions = self._generate_context_suggestions(context, position - start)
            
            return suggestions[:5]  # Return top 5 suggestions
            
        except Exception as e:
            logger.error(f"Suggestion generation error: {e}")
            return []
    
    def _tokenize(self, text: str) -> List[str]:
        """Tokenize text into words."""
        # Simple tokenization - split on whitespace and punctuation
        words = re.findall(r'\b\w+\b', text.lower())
        return words
    
    def _generate_predictions(self, context: List[str], max_predictions: int) -> List[Dict[str, Any]]:
        """Generate word predictions based on context."""
        if not context:
            return []
        
        # Try different context lengths
        predictions = []
        
        # Use last 2 words as context (trigram model)
        if len(context) >= 2:
            context_key = " ".join(context[-2:])
            if context_key in self.ngram_model:
                next_words = self.ngram_model[context_key]
                total_count = sum(next_words.values())
                
                for word, count in next_words.most_common(max_predictions):
                    predictions.append({
                        "word": word,
                        "probability": count / total_count if total_count > 0 else 0
                    })
        
        # Fallback to bigram model
        if len(predictions) < max_predictions and len(context) >= 1:
            context_key = context[-1]
            if context_key in self.ngram_model:
                next_words = self.ngram_model[context_key]
                total_count = sum(next_words.values())
                
                for word, count in next_words.most_common(max_predictions - len(predictions)):
                    predictions.append({
                        "word": word,
                        "probability": count / total_count if total_count > 0 else 0
                    })
        
        # Add some common words if we don't have enough predictions
        if len(predictions) < max_predictions:
            common_words = ["the", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by"]
            for word in common_words:
                if word not in [p["word"] for p in predictions]:
                    predictions.append({
                        "word": word,
                        "probability": 0.1  # Low probability for common words
                    })
                    if len(predictions) >= max_predictions:
                        break
        
        return predictions[:max_predictions]
    
    def _find_completions(self, partial_word: str) -> List[Dict[str, Any]]:
        """Find completions for a partial word."""
        if not partial_word:
            return []
        
        completions = []
        
        # Find words that start with the partial word
        for word in self.vocabulary:
            if word.startswith(partial_word.lower()):
                # Calculate a simple frequency score
                score = self._calculate_word_frequency(word)
                completions.append({
                    "word": word,
                    "probability": score
                })
        
        # Sort by probability and return top completions
        completions.sort(key=lambda x: x["probability"], reverse=True)
        return completions[:5]
    
    def _generate_context_suggestions(self, context: List[str], position: int) -> List[str]:
        """Generate suggestions based on context around a position."""
        suggestions = []
        
        # Use words before and after the position as context
        if position > 0 and position < len(context):
            # Look for patterns in the context
            before_word = context[position - 1] if position > 0 else ""
            after_word = context[position + 1] if position < len(context) - 1 else ""
            
            # Find words that commonly appear between these words
            for context_key, next_words in self.ngram_model.items():
                if before_word in context_key:
                    for word, count in next_words.most_common(3):
                        if word not in suggestions:
                            suggestions.append(word)
        
        return suggestions[:5]
    
    def _calculate_word_frequency(self, word: str) -> float:
        """Calculate frequency score for a word."""
        total_count = 0
        word_count = 0
        
        for context, next_words in self.ngram_model.items():
            total_count += sum(next_words.values())
            word_count += next_words.get(word, 0)
        
        return word_count / total_count if total_count > 0 else 0.001
