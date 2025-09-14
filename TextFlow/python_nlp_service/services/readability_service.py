"""
Readability analysis service using various metrics.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import re
import math

logger = logging.getLogger(__name__)

class ReadabilityService:
    def __init__(self):
        self.initialized = False
    
    async def load_models(self):
        """Load readability analysis models."""
        try:
            self.initialized = True
            logger.info("Readability service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize readability service: {e}")
            raise
    
    async def analyze_readability(self, text: str, metrics: Optional[List[str]] = None) -> Dict[str, Any]:
        """Analyze text readability using various metrics."""
        if not self.initialized:
            await self.load_models()
        
        try:
            if metrics is None:
                metrics = ["flesch_kincaid", "gunning_fog", "smog", "ari", "coleman_liau"]
            
            results = {}
            
            for metric in metrics:
                if metric == "flesch_kincaid":
                    results[metric] = self._flesch_kincaid_grade(text)
                elif metric == "gunning_fog":
                    results[metric] = self._gunning_fog_index(text)
                elif metric == "smog":
                    results[metric] = self._smog_index(text)
                elif metric == "ari":
                    results[metric] = self._automated_readability_index(text)
                elif metric == "coleman_liau":
                    results[metric] = self._coleman_liau_index(text)
                elif metric == "flesch_reading_ease":
                    results[metric] = self._flesch_reading_ease(text)
            
            # Calculate overall grade level
            grade_levels = [score for score in results.values() if isinstance(score, (int, float))]
            avg_grade_level = sum(grade_levels) / len(grade_levels) if grade_levels else 0
            
            # Determine complexity
            complexity = self._determine_complexity(avg_grade_level)
            
            # Generate recommendations
            recommendations = self._generate_recommendations(results, text)
            
            return {
                "scores": results,
                "grade_level": f"Grade {math.ceil(avg_grade_level)}",
                "recommendations": recommendations,
                "complexity": complexity
            }
            
        except Exception as e:
            logger.error(f"Readability analysis error: {e}")
            return {
                "scores": {},
                "grade_level": "Unknown",
                "recommendations": [],
                "complexity": "Unknown"
            }
    
    async def get_improvement_suggestions(self, text: str) -> List[Dict[str, Any]]:
        """Get specific suggestions to improve readability."""
        if not self.initialized:
            await self.load_models()
        
        try:
            suggestions = []
            
            # Analyze sentences
            sentences = self._split_sentences(text)
            long_sentences = [sent for sent in sentences if self._count_words(sent) > 20]
            
            if long_sentences:
                suggestions.append({
                    "type": "long_sentences",
                    "message": f"Consider breaking down {len(long_sentences)} long sentences",
                    "examples": long_sentences[:3],
                    "priority": "high"
                })
            
            # Analyze complex words
            complex_words = self._find_complex_words(text)
            if complex_words:
                suggestions.append({
                    "type": "complex_words",
                    "message": f"Consider simplifying {len(complex_words)} complex words",
                    "examples": complex_words[:5],
                    "priority": "medium"
                })
            
            # Analyze passive voice
            passive_sentences = self._find_passive_voice(text)
            if passive_sentences:
                suggestions.append({
                    "type": "passive_voice",
                    "message": f"Consider using active voice in {len(passive_sentences)} sentences",
                    "examples": passive_sentences[:3],
                    "priority": "medium"
                })
            
            # Analyze paragraph length
            paragraphs = [p.strip() for p in text.split('\n\n') if p.strip()]
            long_paragraphs = [p for p in paragraphs if self._count_words(p) > 150]
            
            if long_paragraphs:
                suggestions.append({
                    "type": "long_paragraphs",
                    "message": f"Consider breaking down {len(long_paragraphs)} long paragraphs",
                    "priority": "low"
                })
            
            return suggestions
            
        except Exception as e:
            logger.error(f"Improvement suggestions error: {e}")
            return []
    
    def _flesch_kincaid_grade(self, text: str) -> float:
        """Calculate Flesch-Kincaid Grade Level."""
        sentences = self._split_sentences(text)
        words = self._tokenize(text)
        syllables = sum(self._count_syllables(word) for word in words)
        
        if len(sentences) == 0 or len(words) == 0:
            return 0
        
        return 0.39 * (len(words) / len(sentences)) + 11.8 * (syllables / len(words)) - 15.59
    
    def _gunning_fog_index(self, text: str) -> float:
        """Calculate Gunning Fog Index."""
        sentences = self._split_sentences(text)
        words = self._tokenize(text)
        complex_words = self._count_complex_words(text)
        
        if len(sentences) == 0 or len(words) == 0:
            return 0
        
        return 0.4 * ((len(words) / len(sentences)) + (100 * complex_words / len(words)))
    
    def _smog_index(self, text: str) -> float:
        """Calculate SMOG Index."""
        sentences = self._split_sentences(text)
        complex_words = self._count_complex_words(text)
        
        if len(sentences) < 30:
            return 0
        
        return 1.043 * math.sqrt(complex_words * (30 / len(sentences))) + 3.1291
    
    def _automated_readability_index(self, text: str) -> float:
        """Calculate Automated Readability Index."""
        sentences = self._split_sentences(text)
        words = self._tokenize(text)
        characters = len(re.sub(r'\s', '', text))
        
        if len(sentences) == 0 or len(words) == 0:
            return 0
        
        return 4.71 * (characters / len(words)) + 0.5 * (len(words) / len(sentences)) - 21.43
    
    def _coleman_liau_index(self, text: str) -> float:
        """Calculate Coleman-Liau Index."""
        sentences = self._split_sentences(text)
        words = self._tokenize(text)
        characters = len(re.sub(r'\s', '', text))
        
        if len(sentences) == 0 or len(words) == 0:
            return 0
        
        L = (characters / len(words)) * 100
        S = (len(sentences) / len(words)) * 100
        
        return 0.0588 * L - 0.296 * S - 15.8
    
    def _flesch_reading_ease(self, text: str) -> float:
        """Calculate Flesch Reading Ease Score."""
        sentences = self._split_sentences(text)
        words = self._tokenize(text)
        syllables = sum(self._count_syllables(word) for word in words)
        
        if len(sentences) == 0 or len(words) == 0:
            return 0
        
        return 206.835 - 1.015 * (len(words) / len(sentences)) - 84.6 * (syllables / len(words))
    
    def _count_syllables(self, word: str) -> int:
        """Count syllables in a word."""
        word = word.lower()
        vowels = "aeiouy"
        syllable_count = 0
        prev_was_vowel = False
        
        for char in word:
            is_vowel = char in vowels
            if is_vowel and not prev_was_vowel:
                syllable_count += 1
            prev_was_vowel = is_vowel
        
        # Handle silent 'e'
        if word.endswith('e') and syllable_count > 1:
            syllable_count -= 1
        
        return max(1, syllable_count)
    
    def _count_complex_words(self, text: str) -> int:
        """Count complex words (3+ syllables)."""
        words = self._tokenize(text)
        return sum(1 for word in words if self._count_syllables(word) >= 3)
    
    def _count_words(self, text: str) -> int:
        """Count words in text."""
        return len(self._tokenize(text))
    
    def _split_sentences(self, text: str) -> List[str]:
        """Split text into sentences."""
        sentences = re.split(r'[.!?]+', text)
        return [s.strip() for s in sentences if s.strip()]
    
    def _tokenize(self, text: str) -> List[str]:
        """Tokenize text into words."""
        words = re.findall(r'\b\w+\b', text.lower())
        return words
    
    def _find_complex_words(self, text: str) -> List[str]:
        """Find complex words in text."""
        words = self._tokenize(text)
        return [word for word in words if self._count_syllables(word) >= 3]
    
    def _find_passive_voice(self, text: str) -> List[str]:
        """Find sentences with passive voice."""
        sentences = self._split_sentences(text)
        passive_sentences = []
        
        for sentence in sentences:
            # Simple heuristic for passive voice
            words = sentence.lower().split()
            if any(be_verb in words for be_verb in ["is", "are", "was", "were", "been", "being"]):
                # Check for past participle pattern
                for i, word in enumerate(words):
                    if (word in ["is", "are", "was", "were", "been", "being"] and
                        i + 1 < len(words) and
                        words[i + 1].endswith(("ed", "en"))):
                        passive_sentences.append(sentence)
                        break
        
        return passive_sentences
    
    def _determine_complexity(self, grade_level: float) -> str:
        """Determine text complexity based on grade level."""
        if grade_level <= 6:
            return "Easy"
        elif grade_level <= 9:
            return "Moderate"
        elif grade_level <= 12:
            return "Difficult"
        else:
            return "Very Difficult"
    
    def _generate_recommendations(self, scores: Dict[str, float], text: str) -> List[str]:
        """Generate readability improvement recommendations."""
        recommendations = []
        
        # Get average grade level
        grade_levels = [score for score in scores.values() if isinstance(score, (int, float))]
        if grade_levels:
            avg_grade = sum(grade_levels) / len(grade_levels)
            
            if avg_grade > 12:
                recommendations.append("Consider simplifying the language for better accessibility")
            elif avg_grade > 9:
                recommendations.append("The text is suitable for high school level readers")
            elif avg_grade > 6:
                recommendations.append("The text is suitable for middle school level readers")
            else:
                recommendations.append("The text is suitable for elementary level readers")
        
        # Check sentence length
        sentences = self._split_sentences(text)
        if sentences:
            avg_sentence_length = sum(self._count_words(sent) for sent in sentences) / len(sentences)
            if avg_sentence_length > 20:
                recommendations.append("Consider shortening sentences for better readability")
        
        # Check complex words
        complex_words = self._find_complex_words(text)
        total_words = len(self._tokenize(text))
        if total_words > 0 and len(complex_words) / total_words > 0.3:
            recommendations.append("Consider replacing complex words with simpler alternatives")
        
        return recommendations
