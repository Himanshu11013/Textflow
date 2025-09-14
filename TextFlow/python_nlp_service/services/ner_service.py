"""
Named Entity Recognition service using spaCy and custom models.
"""

import asyncio
import logging
from typing import Dict, List, Any, Optional
import spacy
from collections import Counter, defaultdict
import re

logger = logging.getLogger(__name__)

class NERService:
    def __init__(self):
        self.nlp = None
        self.initialized = False
    
    async def load_models(self):
        """Load NER models."""
        try:
            # Load spaCy model
            try:
                self.nlp = spacy.load("en_core_web_sm")
            except OSError:
                logger.warning("spaCy model not found. Installing...")
                import subprocess
                subprocess.run(["python", "-m", "spacy", "download", "en_core_web_sm"])
                self.nlp = spacy.load("en_core_web_sm")
            
            self.initialized = True
            logger.info("NER service initialized successfully")
        except Exception as e:
            logger.error(f"Failed to initialize NER service: {e}")
            raise
    
    async def extract_entities(self, text: str, entity_types: Optional[List[str]] = None) -> Dict[str, Any]:
        """Extract named entities from text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            
            entities = []
            entity_counts = defaultdict(int)
            all_entity_types = set()
            
            for ent in doc.ents:
                # Filter by entity types if specified
                if entity_types and ent.label_ not in entity_types:
                    continue
                
                entity_info = {
                    "text": ent.text,
                    "label": ent.label_,
                    "start": ent.start_char,
                    "end": ent.end_char,
                    "description": self._get_entity_description(ent.label_),
                    "confidence": 0.8  # spaCy doesn't provide confidence scores by default
                }
                
                entities.append(entity_info)
                entity_counts[ent.label_] += 1
                all_entity_types.add(ent.label_)
            
            return {
                "entities": entities,
                "entity_types": list(all_entity_types),
                "entity_counts": dict(entity_counts)
            }
            
        except Exception as e:
            logger.error(f"Entity extraction error: {e}")
            return {
                "entities": [],
                "entity_types": [],
                "entity_counts": {}
            }
    
    async def classify_entities(self, text: str) -> Dict[str, Any]:
        """Classify and categorize entities with additional analysis."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            
            # Extract entities
            entities = []
            entity_categories = defaultdict(list)
            
            for ent in doc.ents:
                entity_info = {
                    "text": ent.text,
                    "label": ent.label_,
                    "category": self._categorize_entity(ent.label_),
                    "start": ent.start_char,
                    "end": ent.end_char,
                    "context": self._get_entity_context(text, ent.start_char, ent.end_char)
                }
                
                entities.append(entity_info)
                entity_categories[entity_info["category"]].append(ent.text)
            
            # Analyze entity relationships
            relationships = self._find_entity_relationships(doc, entities)
            
            # Generate insights
            insights = self._generate_entity_insights(entities, entity_categories)
            
            return {
                "entities": entities,
                "categories": dict(entity_categories),
                "relationships": relationships,
                "insights": insights
            }
            
        except Exception as e:
            logger.error(f"Entity classification error: {e}")
            return {
                "entities": [],
                "categories": {},
                "relationships": [],
                "insights": []
            }
    
    async def extract_relationships(self, text: str) -> List[Dict[str, Any]]:
        """Extract relationships between entities."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            relationships = []
            
            # Find subject-verb-object relationships
            for token in doc:
                if token.dep_ == "ROOT" and token.pos_ == "VERB":
                    # Find subject and object
                    subject = None
                    obj = None
                    
                    for child in token.children:
                        if child.dep_ in ["nsubj", "nsubjpass"]:
                            subject = child.text
                        elif child.dep_ in ["dobj", "pobj"]:
                            obj = child.text
                    
                    if subject and obj:
                        relationships.append({
                            "subject": subject,
                            "verb": token.text,
                            "object": obj,
                            "type": "SVO",
                            "confidence": 0.7
                        })
            
            return relationships
            
        except Exception as e:
            logger.error(f"Relationship extraction error: {e}")
            return []
    
    async def extract_locations(self, text: str) -> List[Dict[str, Any]]:
        """Extract and analyze geographical locations."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            locations = []
            
            for ent in doc.ents:
                if ent.label_ in ["GPE", "LOC"]:  # Geopolitical entities and locations
                    location_info = {
                        "name": ent.text,
                        "type": ent.label_,
                        "start": ent.start_char,
                        "end": ent.end_char,
                        "context": self._get_entity_context(text, ent.start_char, ent.end_char),
                        "coordinates": None  # Would require geocoding service
                    }
                    locations.append(location_info)
            
            return locations
            
        except Exception as e:
            logger.error(f"Location extraction error: {e}")
            return []
    
    async def extract_people(self, text: str) -> List[Dict[str, Any]]:
        """Extract and analyze people mentioned in text."""
        if not self.initialized:
            await self.load_models()
        
        try:
            doc = self.nlp(text)
            people = []
            
            for ent in doc.ents:
                if ent.label_ == "PERSON":
                    person_info = {
                        "name": ent.text,
                        "start": ent.start_char,
                        "end": ent.end_char,
                        "context": self._get_entity_context(text, ent.start_char, ent.end_char),
                        "title": self._extract_person_title(ent.text, text),
                        "mentions": self._count_mentions(ent.text, text)
                    }
                    people.append(person_info)
            
            return people
            
        except Exception as e:
            logger.error(f"People extraction error: {e}")
            return []
    
    def _get_entity_description(self, label: str) -> str:
        """Get human-readable description of entity type."""
        descriptions = {
            "PERSON": "Person",
            "ORG": "Organization",
            "GPE": "Geopolitical Entity",
            "LOC": "Location",
            "PRODUCT": "Product",
            "EVENT": "Event",
            "WORK_OF_ART": "Work of Art",
            "LAW": "Law",
            "LANGUAGE": "Language",
            "DATE": "Date",
            "TIME": "Time",
            "MONEY": "Money",
            "PERCENT": "Percentage",
            "QUANTITY": "Quantity",
            "ORDINAL": "Ordinal Number",
            "CARDINAL": "Cardinal Number"
        }
        return descriptions.get(label, "Unknown Entity")
    
    def _categorize_entity(self, label: str) -> str:
        """Categorize entity into broader categories."""
        categories = {
            "PERSON": "People",
            "ORG": "Organizations",
            "GPE": "Places",
            "LOC": "Places",
            "PRODUCT": "Products",
            "EVENT": "Events",
            "WORK_OF_ART": "Creative Works",
            "LAW": "Legal",
            "LANGUAGE": "Languages",
            "DATE": "Temporal",
            "TIME": "Temporal",
            "MONEY": "Financial",
            "PERCENT": "Numerical",
            "QUANTITY": "Numerical",
            "ORDINAL": "Numerical",
            "CARDINAL": "Numerical"
        }
        return categories.get(label, "Other")
    
    def _get_entity_context(self, text: str, start: int, end: int, context_length: int = 50) -> str:
        """Get context around an entity."""
        context_start = max(0, start - context_length)
        context_end = min(len(text), end + context_length)
        
        context = text[context_start:context_end]
        if context_start > 0:
            context = "..." + context
        if context_end < len(text):
            context = context + "..."
        
        return context
    
    def _find_entity_relationships(self, doc, entities: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Find relationships between entities."""
        relationships = []
        
        # Simple co-occurrence analysis
        for i, ent1 in enumerate(entities):
            for j, ent2 in enumerate(entities[i+1:], i+1):
                # Check if entities are in the same sentence
                if self._are_in_same_sentence(doc, ent1["start"], ent2["start"]):
                    relationships.append({
                        "entity1": ent1["text"],
                        "entity2": ent2["text"],
                        "type": "co-occurrence",
                        "confidence": 0.5
                    })
        
        return relationships
    
    def _are_in_same_sentence(self, doc, pos1: int, pos2: int) -> bool:
        """Check if two positions are in the same sentence."""
        for sent in doc.sents:
            if sent.start_char <= pos1 < sent.end_char and sent.start_char <= pos2 < sent.end_char:
                return True
        return False
    
    def _generate_entity_insights(self, entities: List[Dict[str, Any]], 
                                 categories: Dict[str, List[str]]) -> List[str]:
        """Generate insights about entities in the text."""
        insights = []
        
        # Count insights
        total_entities = len(entities)
        if total_entities > 0:
            insights.append(f"Found {total_entities} named entities in the text")
        
        # Category insights
        for category, entity_list in categories.items():
            if len(entity_list) > 1:
                insights.append(f"Multiple {category.lower()} mentioned: {', '.join(set(entity_list))}")
        
        # Most common entity type
        if entities:
            entity_types = [ent["label"] for ent in entities]
            most_common = Counter(entity_types).most_common(1)[0]
            insights.append(f"Most common entity type: {most_common[0]} ({most_common[1]} occurrences)")
        
        return insights
    
    def _extract_person_title(self, person_name: str, text: str) -> Optional[str]:
        """Extract title or role of a person."""
        # Look for common titles before the person's name
        title_patterns = [
            r"(?:Dr\.|Professor|Prof\.|Mr\.|Mrs\.|Ms\.|Miss|Sir|Madam|Captain|Colonel|General|President|CEO|CTO|CFO|Director|Manager|Leader|Chief)\s+" + re.escape(person_name),
            r"(?:Dr\.|Professor|Prof\.|Mr\.|Mrs\.|Ms\.|Miss|Sir|Madam|Captain|Colonel|General|President|CEO|CTO|CFO|Director|Manager|Leader|Chief)\s+\w+\s+" + re.escape(person_name)
        ]
        
        for pattern in title_patterns:
            match = re.search(pattern, text, re.IGNORECASE)
            if match:
                return match.group(1)
        
        return None
    
    def _count_mentions(self, entity_name: str, text: str) -> int:
        """Count how many times an entity is mentioned."""
        return len(re.findall(re.escape(entity_name), text, re.IGNORECASE))
