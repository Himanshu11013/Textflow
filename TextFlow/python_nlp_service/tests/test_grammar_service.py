import pytest
import asyncio
from services.grammar_service import GrammarService

@pytest.fixture
async def grammar_service():
    service = GrammarService()
    await service.load_models()
    return service

@pytest.mark.asyncio
async def test_grammar_check(grammar_service):
    text = "This is a test sentance with errors."
    result = await grammar_service.check_grammar(text)
    
    assert "corrected_text" in result
    assert "errors" in result
    assert "suggestions" in result
    assert len(result["errors"]) > 0

@pytest.mark.asyncio
async def test_auto_correct(grammar_service):
    text = "This is a test sentance."
    corrected = await grammar_service.auto_correct(text)
    
    assert corrected != text
    assert "sentence" in corrected

@pytest.mark.asyncio
async def test_style_suggestions(grammar_service):
    text = "This is a very long sentence that should be broken down into smaller parts for better readability."
    suggestions = await grammar_service.get_style_suggestions(text)
    
    assert len(suggestions) > 0
    assert any(s["type"] == "long_sentences" for s in suggestions)

@pytest.mark.asyncio
async def test_complexity_analysis(grammar_service):
    text = "This is a simple sentence."
    analysis = await grammar_service.analyze_complexity(text)
    
    assert "avg_sentence_length" in analysis
    assert "complex_word_ratio" in analysis
    assert "complexity_score" in analysis
    assert analysis["total_words"] > 0
