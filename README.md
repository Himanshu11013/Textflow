# Intelligent Text Editor

A tab-based intelligent text editor built with Python and PyQt6 that integrates data structures, NLP, security, and systems concepts.

## Design Decisions

### AVL Tree Implementation
- Used an AVL tree to store text content for efficient insertion, deletion, and search operations
- Each node stores a segment of text with its position as the key
- The tree maintains balance factors to ensure O(log n) operations
- Trade-offs: While AVL trees provide guaranteed O(log n) operations, they require more memory and complex implementation compared to simpler data structures

### Comparison with Rope and Gap Buffer
- **Rope Data Structure**: Better for very large documents with frequent edits at random positions, but more complex to implement
- **Gap Buffer**: Excellent for cursor-based editing but less efficient for random access
- **AVL Tree**: Chosen for its balance between implementation complexity and performance for general text editing tasks

### NLP Services
- Used TextBlob and NLTK for grammar checking, spelling correction, and text summarization
- Implemented multithreading to prevent GUI freezing during NLP processing
- Flesch-Kincaid readability scoring for text analysis

### Security Features
- AES encryption using the cryptography library for file security
- Huffman compression for reducing file size
- Separate encryption/compression operations to maintain flexibility

### GUI Design
- Tab-based interface inspired by modern code editors
- Status bar showing performance metrics and editor statistics
- Contextual menus for NLP suggestions

## How to Run

1. Install required dependencies:pip install PyQt6 textblob nltk cryptography
python -m textblob.download_corpora
2. Run the application:
python gui.py

text

## Features

- **Text Editing**: Basic text editing with undo/redo support
- **File Operations**: Open, save, and reopen files with AVL tree rebuilding
- **Encryption/Decryption**: AES encryption for file security
- **Compression**: Huffman compression for reduced file size
- **NLP Services**: Grammar checking, spell checking, text summarization, and readability scoring
- **Performance Tracking**: Operations per second, tree height, and balance factor monitoring

## Limitations

- The AVL tree implementation is simplified and may not be as efficient as production-grade text editors
- NLP features are basic and may not match commercial grammar checkers
- Large files may experience performance issues due to Python's GIL and GUI threading

## Future Enhancements

- Implement a rope data structure for better performance with large documents
- Add syntax highlighting for programming languages
- Integrate with more advanced NLP APIs (e.g., Hugging Face transformers)
- Add collaborative editing features
- Implement version control integration
This implementation provides a complete text editor with all the requested features. The code is organized into three files as specified, uses OOP principles, and includes error handling for various scenarios.

To run the application, you'll need to install the required dependencies and then execute python gui.py. The editor will open with a tab-based interface where you can create, edit, and save files while using the various NLP and security features.

