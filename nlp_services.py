import threading
from textblob import TextBlob
from textblob import Word
import nltk
from nltk.corpus import stopwords
from nltk.tokenize import sent_tokenize, word_tokenize
from collections import defaultdict
import re
import language_tool_python
from spellchecker import SpellChecker

# Download required NLTK data
try:
    nltk.data.find('tokenizers/punkt')
except LookupError:
    nltk.download('punkt')

try:
    nltk.data.find('corpora/stopwords')
except LookupError:
    nltk.download('stopwords')

class NLPServices:
    """NLP services for grammar checking, spelling, summarization, and readability"""
    
    def __init__(self):
        self.thread_pool = []
        self.max_threads = 4
        self.grammar_tool = language_tool_python.LanguageTool('en-US')
        self.spell_checker = SpellChecker()
        
        # Load custom dictionary for common errors
        self.common_errors = {
            'their': ['there', 'they\'re'],
            'there': ['their', 'they\'re'],
            'they\'re': ['their', 'there'],
            'your': ['you\'re'],
            'you\'re': ['your'],
            'its': ['it\'s'],
            'it\'s': ['its'],
            'affect': ['effect'],
            'effect': ['affect'],
            'then': ['than'],
            'than': ['then']
        }
    
    def check_grammar(self, text, callback=None):
        """Check grammar using language_tool_python"""
        def grammar_thread():
            try:
                matches = self.grammar_tool.check(text)
                corrections = []
                
                for match in matches:
                    # Skip spelling errors (we handle them separately)
                    if 'SPELL' in match.ruleId:
                        continue
                        
                    # Find the sentence containing the error
                    sentences = sent_tokenize(text)
                    error_sentence = ""
                    for sentence in sentences:
                        if match.offset < text.find(sentence) + len(sentence):
                            error_sentence = sentence
                            break
                    
                    corrections.append({
                        'sentence': error_sentence,
                        'suggestion': match.replacements[0] if match.replacements else 'No suggestion',
                        'error_type': match.ruleId,
                        'start_pos': match.offset,
                        'end_pos': match.offset + match.errorLength
                    })
                
                if callback:
                    callback(corrections)
                return corrections
            except Exception as e:
                if callback:
                    callback([], str(e))
                return [], str(e)
        
        if callback:
            thread = threading.Thread(target=grammar_thread)
            thread.start()
            self.thread_pool.append(thread)
            # Clean up finished threads
            self.thread_pool = [t for t in self.thread_pool if t.is_alive()]
            return None
        else:
            return grammar_thread()
    
    def check_spelling(self, text, callback=None):
        """Check spelling using SpellChecker and language_tool_python"""
        def spelling_thread():
            try:
                corrections = []
                
                # Use language_tool for spelling
                matches = self.grammar_tool.check(text)
                for match in matches:
                    if 'SPELL' in match.ruleId:
                        word = text[match.offset:match.offset + match.errorLength]
                        suggestions = match.replacements[:3]  # Top 3 suggestions
                        
                        # Add common error suggestions if available
                        if word.lower() in self.common_errors:
                            suggestions = self.common_errors[word.lower()] + suggestions
                        
                        corrections.append({
                            'word': word,
                            'suggestions': suggestions,
                            'start_pos': match.offset,
                            'end_pos': match.offset + match.errorLength
                        })
                
                # Also use SpellChecker for additional checks
                words = word_tokenize(text)
                misspelled = self.spell_checker.unknown(words)
                
                for word in misspelled:
                    # Find all occurrences of this word
                    occurrences = [m.start() for m in re.finditer(r'\b' + re.escape(word) + r'\b', text)]
                    
                    for pos in occurrences:
                        suggestions = self.spell_checker.candidates(word)
                        if suggestions:
                            # Convert set to list and take top 3
                            suggestions = list(suggestions)[:3]
                            
                            # Add common error suggestions if available
                            if word.lower() in self.common_errors:
                                suggestions = self.common_errors[word.lower()] + suggestions
                            
                            corrections.append({
                                'word': word,
                                'suggestions': suggestions,
                                'start_pos': pos,
                                'end_pos': pos + len(word)
                            })
                
                if callback:
                    callback(corrections)
                return corrections
            except Exception as e:
                if callback:
                    callback([], str(e))
                return [], str(e)
        
        if callback:
            thread = threading.Thread(target=spelling_thread)
            thread.start()
            self.thread_pool.append(thread)
            # Clean up finished threads
            self.thread_pool = [t for t in self.thread_pool if t.is_alive()]
            return None
        else:
            return spelling_thread()
    
    def summarize_text(self, text, ratio=0.3, callback=None):
        """Summarize text using extractive summarization"""
        def summarize_thread():
            try:
                # Tokenize the text into sentences
                sentences = sent_tokenize(text)
                
                # Tokenize words and remove stopwords
                stop_words = set(stopwords.words('english'))
                words = word_tokenize(text.lower())
                words = [word for word in words if word.isalnum() and word not in stop_words]
                
                # Calculate word frequency
                word_freq = defaultdict(int)
                for word in words:
                    word_freq[word] += 1
                
                # Normalize frequency
                max_freq = max(word_freq.values()) if word_freq else 1
                for word in word_freq:
                    word_freq[word] = word_freq[word] / max_freq
                
                # Score sentences based on word frequency
                sentence_scores = defaultdict(int)
                for i, sentence in enumerate(sentences):
                    for word in word_tokenize(sentence.lower()):
                        if word in word_freq:
                            sentence_scores[i] += word_freq[word]
                
                # Select top sentences
                num_sentences = max(1, int(len(sentences) * ratio))
                top_sentences = sorted(sentence_scores, key=sentence_scores.get, reverse=True)[:num_sentences]
                
                # Sort selected sentences by their original order
                summary = [sentences[i] for i in sorted(top_sentences)]
                
                if callback:
                    callback(' '.join(summary))
                return ' '.join(summary)
            except Exception as e:
                if callback:
                    callback("", str(e))
                return "", str(e)
        
        if callback:
            thread = threading.Thread(target=summarize_thread)
            thread.start()
            self.thread_pool.append(thread)
            # Clean up finished threads
            self.thread_pool = [t for t in self.thread_pool if t.is_alive()]
            return None
        else:
            return summarize_thread()
    
    def calculate_readability(self, text, callback=None):
        """Calculate Flesch-Kincaid readability score"""
        def readability_thread():
            try:
                sentences = sent_tokenize(text)
                words = word_tokenize(text)
                
                num_sentences = len(sentences)
                num_words = len(words)
                num_syllables = sum(self._count_syllables(word) for word in words)
                
                if num_sentences == 0 or num_words == 0:
                    score = 0
                else:
                    # Flesch-Kincaid readability formula
                    score = 206.835 - 1.015 * (num_words / num_sentences) - 84.6 * (num_syllables / num_words)
                
                # Interpret the score
                if score >= 90:
                    level = "Very Easy (5th grade)"
                elif score >= 80:
                    level = "Easy (6th grade)"
                elif score >= 70:
                    level = "Fairly Easy (7th grade)"
                elif score >= 60:
                    level = "Standard (8th-9th grade)"
                elif score >= 50:
                    level = "Fairly Difficult (10th-12th grade)"
                elif score >= 30:
                    level = "Difficult (College)"
                else:
                    level = "Very Difficult (College graduate)"
                
                result = {
                    'score': score,
                    'level': level,
                    'sentences': num_sentences,
                    'words': num_words,
                    'syllables': num_syllables
                }
                
                if callback:
                    callback(result)
                return result
            except Exception as e:
                if callback:
                    callback({}, str(e))
                return {}, str(e)
        
        if callback:
            thread = threading.Thread(target=readability_thread)
            thread.start()
            self.thread_pool.append(thread)
            # Clean up finished threads
            self.thread_pool = [t for t in self.thread_pool if t.is_alive()]
            return None
        else:
            return readability_thread()
    
    def _count_syllables(self, word):
        """Count syllables in a word - improved method"""
        word = word.lower()
        count = 0
        
        # Remove final 'e'
        if word.endswith('e'):
            word = word[:-1]
        
        # Count vowel groups
        vowels = "aeiouy"
        prev_char_vowel = False
        
        for char in word:
            if char in vowels:
                if not prev_char_vowel:
                    count += 1
                prev_char_vowel = True
            else:
                prev_char_vowel = False
        
        # Ensure at least one syllable
        if count == 0:
            count = 1
        
        return count