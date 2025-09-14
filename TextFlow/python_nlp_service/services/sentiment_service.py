"""
Sentiment analysis service using various techniques.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import re
from collections import Counter
import numpy as np

logger = logging.getLogger(__name__)

class SentimentService:
    def __init__(self):
        self.positive_words = set()
        self.negative_words = set()
        self.intensifiers = set()
        self.negations = set()
        self.initialized = False
    
    async def load_models(self):
        """Load sentiment analysis models."""
        try:
            # Load sentiment lexicons
            self._load_sentiment_lexicons()
            
            self.initialized = True
            logger.info("Sentiment service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize sentiment service: {e}")
            raise
    
    async def analyze_sentiment(self, text: str, granularity: str = "sentence") -> Dict[str, Any]:
        """Analyze sentiment of text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            if granularity == "sentence":
                return await self._analyze_sentence_sentiment(text)
            elif granularity == "paragraph":
                return await self._analyze_paragraph_sentiment(text)
            elif granularity == "document":
                return await self._analyze_document_sentiment(text)
            else:
                raise ValueError(f"Unknown granularity: {granularity}")
                
        except Exception as e:
            logger.error(f"Sentiment analysis error: {e}")
            return {
                "sentiment": "neutral",
                "confidence": 0.5,
                "scores": {"positive": 0.33, "negative": 0.33, "neutral": 0.34},
                "breakdown": []
            }
    
    async def detect_emotions(self, text: str) -> Dict[str, Any]:
        """Detect emotions in text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            emotions = {
                "joy": 0.0,
                "sadness": 0.0,
                "anger": 0.0,
                "fear": 0.0,
                "surprise": 0.0,
                "disgust": 0.0
            }
            
            words = self._tokenize(text.lower())
            
            # Emotion word lists (simplified)
            emotion_words = {
                "joy": ["happy", "joy", "excited", "pleased", "delighted", "cheerful", "glad", "ecstatic"],
                "sadness": ["sad", "depressed", "unhappy", "miserable", "gloomy", "sorrowful", "melancholy"],
                "anger": ["angry", "mad", "furious", "rage", "irritated", "annoyed", "outraged"],
                "fear": ["afraid", "scared", "terrified", "worried", "anxious", "nervous", "frightened"],
                "surprise": ["surprised", "amazed", "shocked", "astonished", "startled", "bewildered"],
                "disgust": ["disgusted", "revolted", "sickened", "repulsed", "appalled", "nauseated"]
            }
            
            # Count emotion words
            for word in words:
                for emotion, emotion_list in emotion_words.items():
                    if word in emotion_list:
                        emotions[emotion] += 1
            
            # Normalize scores
            total_emotion_words = sum(emotions.values())
            if total_emotion_words > 0:
                for emotion in emotions:
                    emotions[emotion] = emotions[emotion] / total_emotion_words
            
            # Find dominant emotion
            dominant_emotion = max(emotions, key=emotions.get)
            
            return {
                "emotions": emotions,
                "dominant_emotion": dominant_emotion,
                "intensity": emotions[dominant_emotion]
            }
            
        except Exception as e:
            logger.error(f"Emotion detection error: {e}")
            return {
                "emotions": {},
                "dominant_emotion": "neutral",
                "intensity": 0.0
            }
    
    async def analyze_tone(self, text: str) -> Dict[str, Any]:
        """Analyze the tone of the text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            words = self._tokenize(text.lower())
            
            # Tone indicators
            formal_words = ["therefore", "however", "furthermore", "moreover", "consequently"]
            informal_words = ["yeah", "ok", "cool", "awesome", "gonna", "wanna"]
            academic_words = ["research", "study", "analysis", "hypothesis", "methodology"]
            casual_words = ["hey", "hi", "thanks", "please", "sorry"]
            
            tone_scores = {
                "formal": 0,
                "informal": 0,
                "academic": 0,
                "casual": 0,
                "professional": 0,
                "friendly": 0
            }
            
            # Count tone indicators
            for word in words:
                if word in formal_words:
                    tone_scores["formal"] += 1
                elif word in informal_words:
                    tone_scores["informal"] += 1
                elif word in academic_words:
                    tone_scores["academic"] += 1
                elif word in casual_words:
                    tone_scores["casual"] += 1
            
            # Analyze punctuation and capitalization
            exclamation_count = text.count('!')
            question_count = text.count('?')
            caps_ratio = sum(1 for c in text if c.isupper()) / len(text) if text else 0
            
            if exclamation_count > 2:
                tone_scores["friendly"] += 1
            if question_count > 3:
                tone_scores["casual"] += 1
            if caps_ratio > 0.1:
                tone_scores["informal"] += 1
            
            # Determine dominant tone
            dominant_tone = max(tone_scores, key=tone_scores.get)
            
            return {
                "tone_scores": tone_scores,
                "dominant_tone": dominant_tone,
                "exclamation_count": exclamation_count,
                "question_count": question_count,
                "caps_ratio": caps_ratio
            }
            
        except Exception as e:
            logger.error(f"Tone analysis error: {e}")
            return {
                "tone_scores": {},
                "dominant_tone": "neutral",
                "exclamation_count": 0,
                "question_count": 0,
                "caps_ratio": 0.0
            }
    
    async def _analyze_sentence_sentiment(self, text: str) -> Dict[str, Any]:
        """Analyze sentiment at sentence level."""
        sentences = self._split_sentences(text)
        sentence_sentiments = []
        
        for sentence in sentences:
            sentiment = self._calculate_sentiment_score(sentence)
            sentence_sentiments.append({
                "text": sentence,
                "sentiment": sentiment["sentiment"],
                "confidence": sentiment["confidence"],
                "scores": sentiment["scores"]
            })
        
        # Aggregate sentence sentiments
        if sentence_sentiments:
            avg_confidence = np.mean([s["confidence"] for s in sentence_sentiments])
            sentiment_counts = Counter([s["sentiment"] for s in sentence_sentiments])
            dominant_sentiment = sentiment_counts.most_common(1)[0][0]
            
            # Calculate overall scores
            positive_score = sum(s["scores"]["positive"] for s in sentence_sentiments) / len(sentence_sentiments)
            negative_score = sum(s["scores"]["negative"] for s in sentence_sentiments) / len(sentence_sentiments)
            neutral_score = sum(s["scores"]["neutral"] for s in sentence_sentiments) / len(sentence_sentiments)
        else:
            avg_confidence = 0.5
            dominant_sentiment = "neutral"
            positive_score = 0.33
            negative_score = 0.33
            neutral_score = 0.34
        
        return {
            "sentiment": dominant_sentiment,
            "confidence": avg_confidence,
            "scores": {
                "positive": positive_score,
                "negative": negative_score,
                "neutral": neutral_score
            },
            "breakdown": sentence_sentiments
        }
    
    async def _analyze_paragraph_sentiment(self, text: str) -> Dict[str, Any]:
        """Analyze sentiment at paragraph level."""
        paragraphs = text.split('\n\n')
        paragraph_sentiments = []
        
        for paragraph in paragraphs:
            if paragraph.strip():
                sentiment = self._calculate_sentiment_score(paragraph)
                paragraph_sentiments.append({
                    "text": paragraph,
                    "sentiment": sentiment["sentiment"],
                    "confidence": sentiment["confidence"],
                    "scores": sentiment["scores"]
                })
        
        # Aggregate paragraph sentiments
        if paragraph_sentiments:
            avg_confidence = np.mean([p["confidence"] for p in paragraph_sentiments])
            sentiment_counts = Counter([p["sentiment"] for p in paragraph_sentiments])
            dominant_sentiment = sentiment_counts.most_common(1)[0][0]
            
            positive_score = sum(p["scores"]["positive"] for p in paragraph_sentiments) / len(paragraph_sentiments)
            negative_score = sum(p["scores"]["negative"] for p in paragraph_sentiments) / len(paragraph_sentiments)
            neutral_score = sum(p["scores"]["neutral"] for p in paragraph_sentiments) / len(paragraph_sentiments)
        else:
            avg_confidence = 0.5
            dominant_sentiment = "neutral"
            positive_score = 0.33
            negative_score = 0.33
            neutral_score = 0.34
        
        return {
            "sentiment": dominant_sentiment,
            "confidence": avg_confidence,
            "scores": {
                "positive": positive_score,
                "negative": negative_score,
                "neutral": neutral_score
            },
            "breakdown": paragraph_sentiments
        }
    
    async def _analyze_document_sentiment(self, text: str) -> Dict[str, Any]:
        """Analyze sentiment at document level."""
        sentiment = self._calculate_sentiment_score(text)
        
        return {
            "sentiment": sentiment["sentiment"],
            "confidence": sentiment["confidence"],
            "scores": sentiment["scores"],
            "breakdown": [{
                "text": text,
                "sentiment": sentiment["sentiment"],
                "confidence": sentiment["confidence"],
                "scores": sentiment["scores"]
            }]
        }
    
    def _calculate_sentiment_score(self, text: str) -> Dict[str, Any]:
        """Calculate sentiment score for a text."""
        words = self._tokenize(text.lower())
        
        positive_score = 0
        negative_score = 0
        neutral_score = 0
        
        for i, word in enumerate(words):
            # Check for negations
            is_negated = False
            if i > 0 and words[i-1] in self.negations:
                is_negated = True
            
            # Check for intensifiers
            intensity = 1.0
            if i > 0 and words[i-1] in self.intensifiers:
                intensity = 1.5
            
            # Calculate word sentiment
            if word in self.positive_words:
                score = 1.0 * intensity
                if is_negated:
                    negative_score += score
                else:
                    positive_score += score
            elif word in self.negative_words:
                score = 1.0 * intensity
                if is_negated:
                    positive_score += score
                else:
                    negative_score += score
            else:
                neutral_score += 1.0
        
        # Normalize scores
        total_score = positive_score + negative_score + neutral_score
        if total_score > 0:
            positive_score /= total_score
            negative_score /= total_score
            neutral_score /= total_score
        
        # Determine sentiment
        if positive_score > negative_score and positive_score > neutral_score:
            sentiment = "positive"
            confidence = positive_score
        elif negative_score > positive_score and negative_score > neutral_score:
            sentiment = "negative"
            confidence = negative_score
        else:
            sentiment = "neutral"
            confidence = neutral_score
        
        return {
            "sentiment": sentiment,
            "confidence": confidence,
            "scores": {
                "positive": positive_score,
                "negative": negative_score,
                "neutral": neutral_score
            }
        }
    
    def _load_sentiment_lexicons(self):
        """Load sentiment lexicons."""
        # Positive words
        self.positive_words = {
            "good", "great", "excellent", "amazing", "wonderful", "fantastic", "awesome",
            "brilliant", "outstanding", "perfect", "beautiful", "lovely", "nice", "happy",
            "joy", "pleasure", "delight", "satisfaction", "success", "victory", "win",
            "love", "like", "enjoy", "appreciate", "admire", "respect", "praise",
            "best", "better", "improve", "enhance", "boost", "increase", "rise",
            "yes", "sure", "definitely", "absolutely", "certainly", "indeed"
        }
        
        # Negative words
        self.negative_words = {
            "bad", "terrible", "awful", "horrible", "disgusting", "hateful", "ugly",
            "worst", "worse", "poor", "disappointing", "frustrating", "annoying",
            "sad", "unhappy", "miserable", "depressed", "angry", "mad", "furious",
            "hate", "dislike", "despise", "loathe", "detest", "abhor", "reject",
            "fail", "failure", "lose", "loss", "defeat", "disaster", "crisis",
            "no", "never", "not", "nothing", "nobody", "nowhere", "none"
        }
        
        # Intensifiers
        self.intensifiers = {
            "very", "extremely", "highly", "completely", "totally", "absolutely",
            "incredibly", "amazingly", "remarkably", "exceptionally", "particularly",
            "really", "truly", "genuinely", "sincerely", "deeply", "profoundly"
        }
        
        # Negations
        self.negations = {
            "not", "no", "never", "nothing", "nobody", "nowhere", "none",
            "neither", "nor", "without", "lack", "missing", "absent"
        }
    
    def _tokenize(self, text: str) -> List[str]:
        """Tokenize text into words."""
        words = re.findall(r'\b\w+\b', text.lower())
        return words
    
    def _split_sentences(self, text: str) -> List[str]:
        """Split text into sentences."""
        sentences = re.split(r'[.!?]+', text)
        return [s.strip() for s in sentences if s.strip()]
