"""
Grammar correction service using LanguageTool and spaCy.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import language_tool_python
import spacy
from spacy import displacy
import re

logger = logging.getLogger(__name__)

class GrammarService:
    def __init__(self):
        self.tool = None
        self.nlp = None
        self.initialized = False
    
    async def load_models(self):
        """Load grammar checking models."""
        try:
            # Load LanguageTool
            self.tool = language_tool_python.LanguageTool('en-US')
            
            # Load spaCy model
            try:
                self.nlp = spacy.load("en_core_web_sm")
            except OSError:
                logger.warning("spaCy model not found. Installing...")
                import subprocess
                subprocess.run(["python", "-m", "spacy", "download", "en_core_web_sm"])
                self.nlp = spacy.load("en_core_web_sm")
            
            self.initialized = True
            logger.info("Grammar service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize grammar service: {e}")
            raise
    
    async def check_grammar(self, text: str, language: str = "en", auto_correct: bool = False) -> Dict[str, Any]:
        """Check grammar and return errors and suggestions."""
        if not self.initialized:
            await self.load_models()
        
        try:
            # Get grammar errors
            matches = self.tool.check(text)
            
            errors = []
            suggestions = []
            
            for match in matches:
                error_info = {
                    "message": match.message,
                    "rule_id": match.ruleId,
                    "offset": match.offset,
                    "length": match.length,
                    "context": match.context,
                    "replacements": match.replacements,
                    "severity": self._get_severity(match.ruleId)
                }
                errors.append(error_info)
                
                if match.replacements:
                    suggestions.extend(match.replacements[:3])  # Top 3 suggestions
            
            # Auto-correct if requested
            corrected_text = text
            if auto_correct and errors:
                corrected_text = self.tool.correct(text)
            
            return {
                "corrected_text": corrected_text,
                "errors": errors,
                "suggestions": list(set(suggestions))[:10]  # Unique suggestions, max 10
            }
            
        except Exception as e:
            logger.error(f"Grammar check error: {e}")
            return {
                "corrected_text": text,
                "errors": [],
                "suggestions": []
            }
    
    async def auto_correct(self, text: str, language: str = "en") -> str:
        """Automatically correct grammar errors."""
        if not self.initialized:
            await self.load_models()
        
        try:
            return self.tool.correct(text)
        except Exception as e:
            logger.error(f"Auto-correction error: {e}")
            return text
    
    async def get_style_suggestions(self, text: str) -> List[Dict[str, Any]]:
        """Get style and writing suggestions."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            suggestions = []
            
            # Check for passive voice
            passive_sentences = self._find_passive_voice(doc)
            if passive_sentences:
                suggestions.append({
                    "type": "passive_voice",
                    "message": "Consider using active voice",
                    "count": len(passive_sentences),
                    "examples": passive_sentences[:3]
                })
            
            # Check for long sentences
            long_sentences = self._find_long_sentences(doc)
            if long_sentences:
                suggestions.append({
                    "type": "long_sentences",
                    "message": "Consider breaking down long sentences",
                    "count": len(long_sentences),
                    "examples": long_sentences[:3]
                })
            
            # Check for repeated words
            repeated_words = self._find_repeated_words(doc)
            if repeated_words:
                suggestions.append({
                    "type": "repetition",
                    "message": "Consider using synonyms for repeated words",
                    "words": repeated_words
                })
            
            return suggestions
            
        except Exception as e:
            logger.error(f"Style suggestions error: {e}")
            return []
    
    async def analyze_complexity(self, text: str) -> Dict[str, Any]:
        """Analyze text complexity and readability."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            
            # Basic statistics
            sentences = [sent.text for sent in doc.sents]
            words = [token.text for token in doc if not token.is_punct and not token.is_space]
            
            avg_sentence_length = len(words) / len(sentences) if sentences else 0
            avg_word_length = sum(len(word) for word in words) / len(words) if words else 0
            
            # Complex word ratio (words with 3+ syllables)
            complex_words = [word for word in words if self._count_syllables(word) >= 3]
            complex_word_ratio = len(complex_words) / len(words) if words else 0
            
            # Passive voice ratio
            passive_sentences = self._find_passive_voice(doc)
            passive_ratio = len(passive_sentences) / len(sentences) if sentences else 0
            
            return {
                "avg_sentence_length": round(avg_sentence_length, 2),
                "avg_word_length": round(avg_word_length, 2),
                "complex_word_ratio": round(complex_word_ratio, 3),
                "passive_ratio": round(passive_ratio, 3),
                "total_words": len(words),
                "total_sentences": len(sentences),
                "complexity_score": self._calculate_complexity_score(
                    avg_sentence_length, complex_word_ratio, passive_ratio
                )
            }
            
        except Exception as e:
            logger.error(f"Complexity analysis error: {e}")
            return {}
    
    def _get_severity(self, rule_id: str) -> str:
        """Determine error severity based on rule ID."""
        high_severity_rules = [
            "MORFOLOGIK_RULE", "SPELLING", "GRAMMAR", "TYPOS"
        ]
        
        if any(rule in rule_id for rule in high_severity_rules):
            return "high"
        elif "STYLE" in rule_id or "REDUNDANCY" in rule_id:
            return "medium"
        else:
            return "low"
    
    def _find_passive_voice(self, doc) -> List[str]:
        """Find sentences with passive voice."""
        passive_sentences = []
        
        for sent in doc.sents:
            # Simple heuristic: look for "be" + past participle
            tokens = [token.text.lower() for token in sent]
            if any(be_verb in tokens for be_verb in ["is", "are", "was", "were", "been", "being"]):
                # Check for past participle pattern
                for i, token in enumerate(sent):
                    if (token.text.lower() in ["is", "are", "was", "were", "been", "being"] and
                        i + 1 < len(sent) and sent[i + 1].tag_ in ["VBN", "VBD"]):
                        passive_sentences.append(sent.text.strip())
                        break
        
        return passive_sentences
    
    def _find_long_sentences(self, doc, max_length: int = 25) -> List[str]:
        """Find sentences that are too long."""
        long_sentences = []
        
        for sent in doc.sents:
            word_count = len([token for token in sent if not token.is_punct and not token.is_space])
            if word_count > max_length:
                long_sentences.append(sent.text.strip())
        
        return long_sentences
    
    def _find_repeated_words(self, doc, min_repetitions: int = 3) -> Dict[str, int]:
        """Find frequently repeated words."""
        word_counts = {}
        
        for token in doc:
            if (not token.is_punct and not token.is_space and 
                not token.is_stop and len(token.text) > 3):
                word = token.text.lower()
                word_counts[word] = word_counts.get(word, 0) + 1
        
        return {word: count for word, count in word_counts.items() 
                if count >= min_repetitions}
    
    def _count_syllables(self, word: str) -> int:
        """Count syllables in a word (approximate)."""
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
    
    def _calculate_complexity_score(self, avg_sentence_length: float, 
                                  complex_word_ratio: float, passive_ratio: float) -> float:
        """Calculate overall complexity score (0-100)."""
        # Weighted combination of complexity factors
        score = (
            min(avg_sentence_length / 20, 1) * 40 +  # Sentence length (max 20 words = 40 points)
            complex_word_ratio * 30 +  # Complex words (30 points max)
            passive_ratio * 30  # Passive voice (30 points max)
        )
        
        return min(100, max(0, score))
