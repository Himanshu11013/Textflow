"""
Text summarization service using various algorithms.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import numpy as np
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity
import spacy
from collections import Counter
import re
import heapq

logger = logging.getLogger(__name__)

class SummarizationService:
    def __init__(self):
        self.nlp = None
        self.vectorizer = None
        self.initialized = False
    
    async def load_models(self):
        """Load summarization models."""
        try:
            # Load spaCy model
            try:
                self.nlp = spacy.load("en_core_web_sm")
            except OSError:
                logger.warning("spaCy model not found. Installing...")
                import subprocess
                subprocess.run(["python", "-m", "spacy", "download", "en_core_web_sm"])
                self.nlp = spacy.load("en_core_web_sm")
            
            # Initialize TF-IDF vectorizer
            self.vectorizer = TfidfVectorizer(
                stop_words='english',
                ngram_range=(1, 2),
                max_features=1000
            )
            
            self.initialized = True
            logger.info("Summarization service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize summarization service: {e}")
            raise
    
    async def summarize(self, text: str, max_length: int = 100, 
                       min_length: int = 20, method: str = "extractive") -> Dict[str, Any]:
        """Summarize text using specified method."""
        if not self.initialized:
            await self.load_models()
        
        try:
            if method == "extractive":
                return await self._extractive_summarize(text, max_length, min_length)
            elif method == "abstractive":
                return await self._abstractive_summarize(text, max_length, min_length)
            elif method == "hybrid":
                return await self._hybrid_summarize(text, max_length, min_length)
            else:
                raise ValueError(f"Unknown summarization method: {method}")
                
        except Exception as e:
            logger.error(f"Summarization error: {e}")
            return {
                "summary": text[:max_length] + "..." if len(text) > max_length else text,
                "original_length": len(text),
                "summary_length": min(len(text), max_length),
                "compression_ratio": min(len(text), max_length) / len(text) if text else 0,
                "key_sentences": []
            }
    
    async def _extractive_summarize(self, text: str, max_length: int, min_length: int) -> Dict[str, Any]:
        """Extractive summarization using TF-IDF and sentence ranking."""
        # Split into sentences
        doc = self.nlp(text)
        sentences = [sent.text.strip() for sent in doc.sents if len(sent.text.strip()) > 10]
        
        if not sentences:
            return {
                "summary": "",
                "original_length": len(text),
                "summary_length": 0,
                "compression_ratio": 0,
                "key_sentences": []
            }
        
        # Calculate sentence scores using TF-IDF
        sentence_scores = self._calculate_sentence_scores(sentences)
        
        # Select top sentences within length constraints
        selected_sentences = self._select_sentences(sentences, sentence_scores, max_length, min_length)
        
        # Create summary
        summary = " ".join(selected_sentences)
        
        return {
            "summary": summary,
            "original_length": len(text),
            "summary_length": len(summary),
            "compression_ratio": len(summary) / len(text) if text else 0,
            "key_sentences": selected_sentences
        }
    
    async def _abstractive_summarize(self, text: str, max_length: int, min_length: int) -> Dict[str, Any]:
        """Abstractive summarization (simplified implementation)."""
        # For now, use extractive as fallback
        # In a full implementation, this would use transformer models
        return await self._extractive_summarize(text, max_length, min_length)
    
    async def _hybrid_summarize(self, text: str, max_length: int, min_length: int) -> Dict[str, Any]:
        """Hybrid summarization combining extractive and abstractive methods."""
        # First, do extractive summarization
        extractive_result = await self._extractive_summarize(text, max_length * 2, min_length)
        
        # Then, apply some abstractive techniques (simplification, paraphrasing)
        summary = self._simplify_text(extractive_result["summary"])
        
        return {
            "summary": summary,
            "original_length": len(text),
            "summary_length": len(summary),
            "compression_ratio": len(summary) / len(text) if text else 0,
            "key_sentences": extractive_result["key_sentences"]
        }
    
    async def extract_keywords(self, text: str, max_keywords: int = 10) -> List[str]:
        """Extract keywords from text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            
            # Extract noun phrases and named entities
            keywords = []
            
            # Add named entities
            for ent in doc.ents:
                if ent.label_ in ["PERSON", "ORG", "GPE", "PRODUCT", "EVENT", "WORK_OF_ART"]:
                    keywords.append(ent.text)
            
            # Add noun phrases
            for chunk in doc.noun_chunks:
                if len(chunk.text.split()) <= 3:  # Max 3 words
                    keywords.append(chunk.text)
            
            # Add important words (nouns, adjectives)
            for token in doc:
                if (token.pos_ in ["NOUN", "ADJ"] and 
                    not token.is_stop and 
                    not token.is_punct and 
                    len(token.text) > 2):
                    keywords.append(token.text)
            
            # Count and rank keywords
            keyword_counts = Counter(keywords)
            
            # Return top keywords
            return [word for word, count in keyword_counts.most_common(max_keywords)]
            
        except Exception as e:
            logger.error(f"Keyword extraction error: {e}")
            return []
    
    async def extract_phrases(self, text: str, min_frequency: int = 2) -> List[Dict[str, Any]]:
        """Extract important phrases from text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            
            # Extract noun phrases
            phrases = []
            for chunk in doc.noun_chunks:
                phrase = chunk.text.strip()
                if len(phrase.split()) >= 2 and len(phrase) > 5:
                    phrases.append(phrase)
            
            # Count phrase frequency
            phrase_counts = Counter(phrases)
            
            # Filter by minimum frequency and return with scores
            result = []
            for phrase, count in phrase_counts.items():
                if count >= min_frequency:
                    result.append({
                        "phrase": phrase,
                        "frequency": count,
                        "score": count / len(phrases) if phrases else 0
                    })
            
            # Sort by score
            result.sort(key=lambda x: x["score"], reverse=True)
            
            return result
            
        except Exception as e:
            logger.error(f"Phrase extraction error: {e}")
            return []
    
    def _calculate_sentence_scores(self, sentences: List[str]) -> List[float]:
        """Calculate TF-IDF scores for sentences."""
        if not sentences:
            return []
        
        try:
            # Fit TF-IDF vectorizer
            tfidf_matrix = self.vectorizer.fit_transform(sentences)
            
            # Calculate sentence scores
            scores = []
            for i, sentence in enumerate(sentences):
                # Sum of TF-IDF scores for words in sentence
                score = np.sum(tfidf_matrix[i].toarray())
                
                # Normalize by sentence length
                word_count = len(sentence.split())
                normalized_score = score / word_count if word_count > 0 else 0
                
                scores.append(normalized_score)
            
            return scores
            
        except Exception as e:
            logger.error(f"Sentence scoring error: {e}")
            return [0.0] * len(sentences)
    
    def _select_sentences(self, sentences: List[str], scores: List[float], 
                         max_length: int, min_length: int) -> List[str]:
        """Select sentences for summary based on scores and length constraints."""
        if not sentences or not scores:
            return []
        
        # Create (score, index) pairs
        scored_sentences = [(scores[i], i, sentences[i]) for i in range(len(sentences))]
        
        # Sort by score (descending)
        scored_sentences.sort(key=lambda x: x[0], reverse=True)
        
        selected = []
        current_length = 0
        
        for score, index, sentence in scored_sentences:
            sentence_length = len(sentence)
            
            # Check if adding this sentence would exceed max_length
            if current_length + sentence_length <= max_length:
                selected.append(sentence)
                current_length += sentence_length
            else:
                # If we haven't reached min_length, add partial sentence
                if current_length < min_length:
                    remaining = max_length - current_length
                    if remaining > 50:  # Only add if there's meaningful content
                        selected.append(sentence[:remaining] + "...")
                break
        
        return selected
    
    def _simplify_text(self, text: str) -> str:
        """Simplify text by replacing complex words and structures."""
        if not self.initialized:
            return text
        
        try:
            doc = self.nlp(text)
            simplified_sentences = []
            
            for sent in doc.sents:
                simplified_sent = self._simplify_sentence(sent)
                simplified_sentences.append(simplified_sent)
            
            return " ".join(simplified_sentences)
            
        except Exception as e:
            logger.error(f"Text simplification error: {e}")
            return text
    
    def _simplify_sentence(self, sent) -> str:
        """Simplify a single sentence."""
        # Simple simplification rules
        simplified = sent.text
        
        # Replace complex words with simpler alternatives
        replacements = {
            "utilize": "use",
            "facilitate": "help",
            "implement": "do",
            "commence": "start",
            "terminate": "end",
            "ascertain": "find out",
            "endeavor": "try",
            "subsequent": "next",
            "prior": "before",
            "numerous": "many"
        }
        
        for complex_word, simple_word in replacements.items():
            simplified = re.sub(r'\b' + complex_word + r'\b', simple_word, simplified, flags=re.IGNORECASE)
        
        return simplified
