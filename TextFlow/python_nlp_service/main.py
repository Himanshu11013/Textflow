#!/usr/bin/env python3
"""
TextFlow NLP Service
A FastAPI-based microservice providing NLP capabilities for the TextFlow editor.
"""

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import List, Dict, Any, Optional
import uvicorn
import logging
import asyncio
from contextlib import asynccontextmanager

from services.grammar_service import GrammarService
from services.summarization_service import SummarizationService
from services.ner_service import NERService
from services.prediction_service import PredictionService
from services.sentiment_service import SentimentService
from services.readability_service import ReadabilityService

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Global service instances
grammar_service = None
summarization_service = None
ner_service = None
prediction_service = None
sentiment_service = None
readability_service = None

@asynccontextmanager
async def lifespan(app: FastAPI):
    """Initialize services on startup and cleanup on shutdown."""
    global grammar_service, summarization_service, ner_service
    global prediction_service, sentiment_service, readability_service
    
    logger.info("Initializing NLP services...")
    
    try:
        # Initialize all services
        grammar_service = GrammarService()
        summarization_service = SummarizationService()
        ner_service = NERService()
        prediction_service = PredictionService()
        sentiment_service = SentimentService()
        readability_service = ReadabilityService()
        
        # Load models asynchronously
        await asyncio.gather(
            grammar_service.load_models(),
            summarization_service.load_models(),
            ner_service.load_models(),
            prediction_service.load_models(),
            sentiment_service.load_models(),
            readability_service.load_models()
        )
        
        logger.info("All NLP services initialized successfully")
        yield
        
    except Exception as e:
        logger.error(f"Failed to initialize services: {e}")
        raise
    finally:
        logger.info("Shutting down NLP services...")

# Create FastAPI app
app = FastAPI(
    title="TextFlow NLP Service",
    description="Advanced NLP capabilities for the TextFlow text editor",
    version="1.0.0",
    lifespan=lifespan
)

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Pydantic models for request/response
class TextRequest(BaseModel):
    text: str
    options: Optional[Dict[str, Any]] = {}

class GrammarRequest(BaseModel):
    text: str
    language: str = "en"
    auto_correct: bool = False

class GrammarResponse(BaseModel):
    corrected_text: str
    errors: List[Dict[str, Any]]
    suggestions: List[str]

class SummarizationRequest(BaseModel):
    text: str
    max_length: int = 100
    min_length: int = 20
    method: str = "extractive"  # extractive, abstractive, or hybrid

class SummarizationResponse(BaseModel):
    summary: str
    original_length: int
    summary_length: int
    compression_ratio: float
    key_sentences: List[str]

class NERRequest(BaseModel):
    text: str
    entities: Optional[List[str]] = None  # Filter specific entity types

class NERResponse(BaseModel):
    entities: List[Dict[str, Any]]
    entity_types: List[str]
    entity_counts: Dict[str, int]

class PredictionRequest(BaseModel):
    text: str
    max_predictions: int = 5
    context_length: int = 50

class PredictionResponse(BaseModel):
    predictions: List[str]
    probabilities: List[float]
    context: str

class SentimentRequest(BaseModel):
    text: str
    granularity: str = "sentence"  # sentence, paragraph, or document

class SentimentResponse(BaseModel):
    sentiment: str
    confidence: float
    scores: Dict[str, float]
    breakdown: List[Dict[str, Any]]

class ReadabilityRequest(BaseModel):
    text: str
    metrics: Optional[List[str]] = None

class ReadabilityResponse(BaseModel):
    scores: Dict[str, float]
    grade_level: str
    recommendations: List[str]
    complexity: str

# Health check endpoint
@app.get("/health")
async def health_check():
    """Health check endpoint."""
    return {
        "status": "healthy",
        "services": {
            "grammar": grammar_service is not None,
            "summarization": summarization_service is not None,
            "ner": ner_service is not None,
            "prediction": prediction_service is not None,
            "sentiment": sentiment_service is not None,
            "readability": readability_service is not None
        }
    }

# Grammar correction endpoints
@app.post("/grammar/check", response_model=GrammarResponse)
async def check_grammar(request: GrammarRequest):
    """Check grammar and provide corrections."""
    try:
        result = await grammar_service.check_grammar(
            request.text, 
            request.language, 
            request.auto_correct
        )
        return GrammarResponse(**result)
    except Exception as e:
        logger.error(f"Grammar check error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/grammar/correct")
async def correct_grammar(request: GrammarRequest):
    """Automatically correct grammar errors."""
    try:
        result = await grammar_service.auto_correct(
            request.text, 
            request.language
        )
        return {"corrected_text": result}
    except Exception as e:
        logger.error(f"Grammar correction error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Summarization endpoints
@app.post("/summarize", response_model=SummarizationResponse)
async def summarize_text(request: SummarizationRequest):
    """Summarize text using various methods."""
    try:
        result = await summarization_service.summarize(
            request.text,
            request.max_length,
            request.min_length,
            request.method
        )
        return SummarizationResponse(**result)
    except Exception as e:
        logger.error(f"Summarization error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/summarize/keywords")
async def extract_keywords(request: TextRequest):
    """Extract keywords from text."""
    try:
        keywords = await summarization_service.extract_keywords(request.text)
        return {"keywords": keywords}
    except Exception as e:
        logger.error(f"Keyword extraction error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Named Entity Recognition endpoints
@app.post("/ner/extract", response_model=NERResponse)
async def extract_entities(request: NERRequest):
    """Extract named entities from text."""
    try:
        result = await ner_service.extract_entities(
            request.text, 
            request.entities
        )
        return NERResponse(**result)
    except Exception as e:
        logger.error(f"NER extraction error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/ner/classify")
async def classify_entities(request: NERRequest):
    """Classify and categorize entities."""
    try:
        result = await ner_service.classify_entities(request.text)
        return result
    except Exception as e:
        logger.error(f"Entity classification error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Text prediction endpoints
@app.post("/predict/next", response_model=PredictionResponse)
async def predict_next_words(request: PredictionRequest):
    """Predict next words based on context."""
    try:
        result = await prediction_service.predict_next_words(
            request.text,
            request.max_predictions,
            request.context_length
        )
        return PredictionResponse(**result)
    except Exception as e:
        logger.error(f"Text prediction error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/predict/complete")
async def complete_text(request: TextRequest):
    """Complete incomplete text."""
    try:
        result = await prediction_service.complete_text(request.text)
        return {"completed_text": result}
    except Exception as e:
        logger.error(f"Text completion error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Sentiment analysis endpoints
@app.post("/sentiment/analyze", response_model=SentimentResponse)
async def analyze_sentiment(request: SentimentRequest):
    """Analyze sentiment of text."""
    try:
        result = await sentiment_service.analyze_sentiment(
            request.text,
            request.granularity
        )
        return SentimentResponse(**result)
    except Exception as e:
        logger.error(f"Sentiment analysis error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/sentiment/emotions")
async def detect_emotions(request: TextRequest):
    """Detect emotions in text."""
    try:
        emotions = await sentiment_service.detect_emotions(request.text)
        return {"emotions": emotions}
    except Exception as e:
        logger.error(f"Emotion detection error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Readability analysis endpoints
@app.post("/readability/analyze", response_model=ReadabilityResponse)
async def analyze_readability(request: ReadabilityRequest):
    """Analyze text readability."""
    try:
        result = await readability_service.analyze_readability(
            request.text,
            request.metrics
        )
        return ReadabilityResponse(**result)
    except Exception as e:
        logger.error(f"Readability analysis error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/readability/improve")
async def improve_readability(request: TextRequest):
    """Get suggestions to improve readability."""
    try:
        suggestions = await readability_service.get_improvement_suggestions(request.text)
        return {"suggestions": suggestions}
    except Exception as e:
        logger.error(f"Readability improvement error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

# Batch processing endpoint
@app.post("/batch/process")
async def batch_process(request: Dict[str, Any]):
    """Process multiple NLP tasks in batch."""
    try:
        text = request.get("text", "")
        tasks = request.get("tasks", [])
        
        results = {}
        
        for task in tasks:
            task_type = task.get("type")
            task_options = task.get("options", {})
            
            if task_type == "grammar":
                results[task_type] = await grammar_service.check_grammar(text, **task_options)
            elif task_type == "summarize":
                results[task_type] = await summarization_service.summarize(text, **task_options)
            elif task_type == "ner":
                results[task_type] = await ner_service.extract_entities(text, **task_options)
            elif task_type == "sentiment":
                results[task_type] = await sentiment_service.analyze_sentiment(text, **task_options)
            elif task_type == "readability":
                results[task_type] = await readability_service.analyze_readability(text, **task_options)
        
        return {"results": results}
    except Exception as e:
        logger.error(f"Batch processing error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(
        "main:app",
        host="0.0.0.0",
        port=8000,
        reload=True,
        log_level="info"
    )
