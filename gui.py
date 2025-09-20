import sys
import os
from PyQt6.QtWidgets import (QApplication, QMainWindow, QTextEdit, QTabWidget, QVBoxLayout, 
                             QWidget, QMenuBar, QMenu, QStatusBar, QFileDialog, QMessageBox,
                             QToolBar, QDialog, QLabel, QPushButton, QHBoxLayout,
                             QListWidget, QListWidgetItem, QDialogButtonBox, QPlainTextEdit, QInputDialog)
from PyQt6.QtGui import QIcon, QTextCursor, QTextCharFormat, QColor, QAction, QKeySequence, QFont, QTextFormat
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QTimer
import json
import threading
from editor_core import EditorCore
from nlp_services import NLPServices
import time

class WorkerThread(QThread):
    """Thread for running NLP tasks without freezing the GUI"""
    finished = pyqtSignal(object)
    error = pyqtSignal(str)
    
    def __init__(self, function, *args, **kwargs):
        super().__init__()
        self.function = function
        self.args = args
        self.kwargs = kwargs
    
    def run(self):
        try:
            result = self.function(*self.args, **self.kwargs)
            self.finished.emit(result)
        except Exception as e:
            self.error.emit(str(e))

class TextEditor(QMainWindow):
    """Main text editor window with tabs"""
    
    def __init__(self):
        super().__init__()
        self.editor_core = EditorCore()
        self.nlp_services = NLPServices()
        self.current_file_paths = {}  # tab_index -> file_path
        self.unsaved_changes = {}  # tab_index -> bool
        self.setWindowTitle("Intelligent Text Editor")
        self.setGeometry(100, 100, 800, 600)
        
        self.init_ui()
    
    def init_ui(self):
        # Create central widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)
        
        # Create status bar early so it's available to other slots
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")
        
        # Create tab widget
        self.tab_widget = QTabWidget()
        self.tab_widget.setTabsClosable(True)
        self.tab_widget.tabCloseRequested.connect(self.close_tab)
        self.tab_widget.currentChanged.connect(self.tab_changed)
        layout.addWidget(self.tab_widget)
        
        # Create initial tab
        self.add_new_tab()
        
        # Create menu bar
        self.create_menu_bar()
        
        # Create toolbar
        self.create_toolbar()
        
        # Update performance stats periodically using a GUI-safe timer
        self.stats_timer = QTimer(self)
        self.stats_timer.setInterval(5000)
        self.stats_timer.timeout.connect(self.update_stats)
        self.stats_timer.start()
    
    def create_menu_bar(self):
        menu_bar = QMenuBar(self)
        self.setMenuBar(menu_bar)
        
        # File menu
        file_menu = QMenu("File", self)
        menu_bar.addMenu(file_menu)
        
        new_action = QAction("New", self)
        new_action.setShortcut(QKeySequence.StandardKey.New)
        new_action.triggered.connect(self.new_file)
        file_menu.addAction(new_action)
        
        open_action = QAction("Open", self)
        open_action.setShortcut(QKeySequence.StandardKey.Open)
        open_action.triggered.connect(self.open_file)
        file_menu.addAction(open_action)
        
        save_action = QAction("Save", self)
        save_action.setShortcut(QKeySequence.StandardKey.Save)
        save_action.triggered.connect(self.save_file)
        file_menu.addAction(save_action)
        
        save_as_action = QAction("Save As", self)
        save_as_action.setShortcut(QKeySequence.StandardKey.SaveAs)
        save_as_action.triggered.connect(self.save_file_as)
        file_menu.addAction(save_as_action)
        
        file_menu.addSeparator()
        
        encrypt_action = QAction("Encrypt", self)
        encrypt_action.triggered.connect(self.encrypt_file)
        file_menu.addAction(encrypt_action)
        
        decrypt_action = QAction("Decrypt", self)
        decrypt_action.triggered.connect(self.decrypt_file)
        file_menu.addAction(decrypt_action)
        
        compress_action = QAction("Compress", self)
        compress_action.triggered.connect(self.compress_file)
        file_menu.addAction(compress_action)
        
        decompress_action = QAction("Decompress", self)
        decompress_action.triggered.connect(self.decompress_file)
        file_menu.addAction(decompress_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("Exit", self)
        exit_action.setShortcut(QKeySequence.StandardKey.Quit)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Edit menu
        edit_menu = QMenu("Edit", self)
        menu_bar.addMenu(edit_menu)
        
        undo_action = QAction("Undo", self)
        undo_action.setShortcut(QKeySequence.StandardKey.Undo)
        undo_action.triggered.connect(self.undo)
        edit_menu.addAction(undo_action)
        
        redo_action = QAction("Redo", self)
        redo_action.setShortcut(QKeySequence.StandardKey.Redo)
        redo_action.triggered.connect(self.redo)
        edit_menu.addAction(redo_action)
        
        # Tools menu
        tools_menu = QMenu("Tools", self)
        menu_bar.addMenu(tools_menu)
        
        grammar_action = QAction("Check Grammar", self)
        grammar_action.triggered.connect(self.check_grammar)
        tools_menu.addAction(grammar_action)
        
        spell_action = QAction("Check Spelling", self)
        spell_action.triggered.connect(self.check_spelling)
        tools_menu.addAction(spell_action)
        
        summarize_action = QAction("Summarize", self)
        summarize_action.triggered.connect(self.summarize_text)
        tools_menu.addAction(summarize_action)
        
        readability_action = QAction("Readability Score", self)
        readability_action.triggered.connect(self.calculate_readability)
        tools_menu.addAction(readability_action)
    
    def create_toolbar(self):
        toolbar = QToolBar("Main Toolbar")
        self.addToolBar(toolbar)
        
        new_action = QAction(QIcon.fromTheme("document-new"), "New", self)
        new_action.triggered.connect(self.new_file)
        toolbar.addAction(new_action)
        
        open_action = QAction(QIcon.fromTheme("document-open"), "Open", self)
        open_action.triggered.connect(self.open_file)
        toolbar.addAction(open_action)
        
        save_action = QAction(QIcon.fromTheme("document-save"), "Save", self)
        save_action.triggered.connect(self.save_file)
        toolbar.addAction(save_action)
        
        toolbar.addSeparator()
        
        undo_action = QAction(QIcon.fromTheme("edit-undo"), "Undo", self)
        undo_action.triggered.connect(self.undo)
        toolbar.addAction(undo_action)
        
        redo_action = QAction(QIcon.fromTheme("edit-redo"), "Redo", self)
        redo_action.triggered.connect(self.redo)
        toolbar.addAction(redo_action)
        
        toolbar.addSeparator()
        
        grammar_action = QAction(QIcon.fromTheme("tools-check-spelling"), "Grammar", self)
        grammar_action.triggered.connect(self.check_grammar)
        toolbar.addAction(grammar_action)
        
        spell_action = QAction(QIcon.fromTheme("tools-check-spelling"), "Spelling", self)
        spell_action.triggered.connect(self.check_spelling)
        toolbar.addAction(spell_action)
    
    def add_new_tab(self, file_path=None, content=""):
        text_edit = QTextEdit()
        text_edit.textChanged.connect(self.text_changed)
        
        if content:
            text_edit.setPlainText(content)
        
        tab_index = self.tab_widget.addTab(text_edit, "Untitled" if not file_path else os.path.basename(file_path))
        self.tab_widget.setCurrentIndex(tab_index)
        
        if file_path:
            self.current_file_paths[tab_index] = file_path
        else:
            self.current_file_paths[tab_index] = None
        
        self.unsaved_changes[tab_index] = False
        
        return tab_index
    
    def close_tab(self, index):
        if self.unsaved_changes.get(index, False):
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "You have unsaved changes. Do you want to save before closing?",
                QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel
            )
            
            if reply == QMessageBox.StandardButton.Save:
                if not self.save_file():
                    return  # User cancelled save
            
            elif reply == QMessageBox.StandardButton.Cancel:
                return  # Don't close the tab
        
        self.tab_widget.removeTab(index)
        
        # Update dictionaries
        if index in self.current_file_paths:
            del self.current_file_paths[index]
        if index in self.unsaved_changes:
            del self.unsaved_changes[index]
        
        # If no tabs left, create a new one
        if self.tab_widget.count() == 0:
            self.add_new_tab()
    
    def tab_changed(self, index):
        if index >= 0:
            self.update_status_bar()
    
    def text_changed(self):
        current_index = self.tab_widget.currentIndex()
        if current_index >= 0:
            self.unsaved_changes[current_index] = True
            title = self.tab_widget.tabText(current_index)
            if not title.endswith('*'):
                self.tab_widget.setTabText(current_index, title + '*')
            
            # Update the AVL tree with the new content
            content = self.get_current_content()
            self.editor_core.avl_tree = EditorCore().avl_tree
            self.editor_core.insert_text(0, content)
    
    def get_current_editor(self):
        current_index = self.tab_widget.currentIndex()
        if current_index >= 0:
            return self.tab_widget.widget(current_index)
        return None
    
    def get_current_content(self):
        editor = self.get_current_editor()
        if editor:
            return editor.toPlainText()
        return ""
    
    def set_current_content(self, content):
        editor = self.get_current_editor()
        if editor:
            editor.setPlainText(content)
    
    def new_file(self):
        self.add_new_tab()
    
    def open_file(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Open File", "", "Text Files (*.txt);;All Files (*)"
        )
        
        if file_path:
            success, message = self.editor_core.open_file(file_path)
            if success:
                content = self.editor_core.avl_tree.get_text()
                self.add_new_tab(file_path, content)
                self.status_bar.showMessage(f"Opened: {file_path}")
            else:
                QMessageBox.critical(self, "Error", message)
    
    def save_file(self):
        current_index = self.tab_widget.currentIndex()
        if current_index < 0:
            return False
        
        file_path = self.current_file_paths.get(current_index)
        
        if file_path:
            content = self.get_current_content()
            success, message = self.editor_core.save_file(file_path, content)
            
            if success:
                self.unsaved_changes[current_index] = False
                title = self.tab_widget.tabText(current_index).rstrip('*')
                self.tab_widget.setTabText(current_index, title)
                self.status_bar.showMessage(f"Saved: {file_path}")
            else:
                QMessageBox.critical(self, "Error", message)
            
            return success
        else:
            return self.save_file_as()
    
    def save_file_as(self):
        current_index = self.tab_widget.currentIndex()
        if current_index < 0:
            return False
        
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save File", "", "Text Files (*.txt);;All Files (*)"
        )
        
        if file_path:
            content = self.get_current_content()
            success, message = self.editor_core.save_file(file_path, content)
            
            if success:
                self.current_file_paths[current_index] = file_path
                self.unsaved_changes[current_index] = False
                self.tab_widget.setTabText(current_index, os.path.basename(file_path))
                self.status_bar.showMessage(f"Saved: {file_path}")
            else:
                QMessageBox.critical(self, "Error", message)
            
            return success
        
        return False
    
    def encrypt_file(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to encrypt")
            return
        
        success, encrypted_content, key = self.editor_core.encrypt_file(content)
        
        if success:
            # Show key to user
            key_str = key.decode() if isinstance(key, bytes) else str(key)
            QMessageBox.information(
                self, "Encryption Key",
                f"File encrypted successfully.\n\nKeep this key safe:\n{key_str}"
            )
            
            # Update content with encrypted data (as hex for display)
            hex_content = encrypted_content.hex()
            self.set_current_content(hex_content)
            
            current_index = self.tab_widget.currentIndex()
            if current_index >= 0:
                self.unsaved_changes[current_index] = True
                title = self.tab_widget.tabText(current_index)
                if not title.endswith('*'):
                    self.tab_widget.setTabText(current_index, title + '*')
        else:
            QMessageBox.critical(self, "Error", encrypted_content)
    
    def decrypt_file(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to decrypt")
            return
        
        # Get key from user
        key, ok = QInputDialog.getText(
            self, "Decryption Key", "Enter encryption key:"
        )
        
        if ok and key:
            try:
                # Try to convert hex content back to bytes
                encrypted_content = bytes.fromhex(content)
                success, decrypted_content = self.editor_core.decrypt_file(encrypted_content, key)
                
                if success:
                    self.set_current_content(decrypted_content)
                    current_index = self.tab_widget.currentIndex()
                    if current_index >= 0:
                        self.unsaved_changes[current_index] = True
                        title = self.tab_widget.tabText(current_index)
                        if not title.endswith('*'):
                            self.tab_widget.setTabText(current_index, title + '*')
                else:
                    QMessageBox.critical(self, "Error", decrypted_content)
            except ValueError:
                QMessageBox.critical(self, "Error", "Content is not in valid hex format")
        else:
            QMessageBox.information(self, "Info", "Decryption cancelled")
    
    def compress_file(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to compress")
            return
        
        success, compressed_content, metadata = self.editor_core.compress_content(content)
        
        if success:
            # Convert compressed bytes to hex for display
            hex_content = compressed_content.hex()
            # Store metadata as JSON string
            metadata_str = json.dumps(metadata)
            # Combine both with a separator
            combined = f"COMPRESSED:::{metadata_str}:::{hex_content}"
            
            self.set_current_content(combined)
            
            current_index = self.tab_widget.currentIndex()
            if current_index >= 0:
                self.unsaved_changes[current_index] = True
                title = self.tab_widget.tabText(current_index)
                if not title.endswith('*'):
                    self.tab_widget.setTabText(current_index, title + '*')
            
            # Show compression ratio
            original_size = len(content)
            compressed_size = len(combined)
            ratio = compressed_size / original_size if original_size > 0 else 0
            QMessageBox.information(
                self, "Compression Complete",
                f"Compression completed.\n\nOriginal size: {original_size} bytes\n"
                f"Compressed size: {compressed_size} bytes\n"
                f"Compression ratio: {ratio:.2%}"
            )
        else:
            QMessageBox.critical(self, "Error", compressed_content)
    
    def decompress_file(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to decompress")
            return
        
        if not content.startswith("COMPRESSED:::"):
            QMessageBox.critical(self, "Error", "Content is not in compressed format")
            return
        
        try:
            # Extract metadata and compressed data
            parts = content.split(":::", 2)
            if len(parts) < 3:
                raise ValueError("Invalid compressed format")
            
            metadata = json.loads(parts[1])
            hex_content = parts[2]
            compressed_content = bytes.fromhex(hex_content)
            
            success, decompressed_content = self.editor_core.decompress_content(compressed_content, metadata)
            
            if success:
                self.set_current_content(decompressed_content)
                current_index = self.tab_widget.currentIndex()
                if current_index >= 0:
                    self.unsaved_changes[current_index] = True
                    title = self.tab_widget.tabText(current_index)
                    if not title.endswith('*'):
                        self.tab_widget.setTabText(current_index, title + '*')
            else:
                QMessageBox.critical(self, "Error", decompressed_content)
        except (ValueError, json.JSONDecodeError) as e:
            QMessageBox.critical(self, "Error", f"Invalid compressed format: {str(e)}")
    
    def undo(self):
        if self.editor_core.undo():
            content = self.editor_core.avl_tree.get_text()
            self.set_current_content(content)
            self.status_bar.showMessage("Undo performed")
        else:
            self.status_bar.showMessage("Nothing to undo")
    
    def redo(self):
        if self.editor_core.redo():
            content = self.editor_core.avl_tree.get_text()
            self.set_current_content(content)
            self.status_bar.showMessage("Redo performed")
        else:
            self.status_bar.showMessage("Nothing to redo")
    
    def check_grammar(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to check")
            return
        
        self.status_bar.showMessage("Checking grammar...")
        
        # Run in thread to avoid freezing GUI
        self.grammar_thread = WorkerThread(self.nlp_services.check_grammar, content)
        self.grammar_thread.finished.connect(self.grammar_check_finished)
        self.grammar_thread.error.connect(self.nlp_error)
        self.grammar_thread.start()
    
    def grammar_check_finished(self, corrections):
        if not corrections:
            QMessageBox.information(self, "Grammar Check", "No grammar issues found")
            self.status_bar.showMessage("Grammar check completed - no issues found")
            return
        
        # Highlight grammar errors in the text
        editor = self.get_current_editor()
        if editor:
            # Clear previous highlights
            cursor = editor.textCursor()
            cursor.select(QTextCursor.SelectionType.Document)
            format = QTextCharFormat()
            format.setBackground(QColor("white"))
            cursor.mergeCharFormat(format)
            
            # Highlight new errors
            format.setBackground(QColor("yellow"))
            for error in corrections:
                cursor.setPosition(error['start_pos'])
                cursor.setPosition(error['end_pos'], QTextCursor.MoveMode.KeepAnchor)
                cursor.mergeCharFormat(format)
        
        # Show grammar issues in a dialog
        dialog = QDialog(self)
        dialog.setWindowTitle("Grammar Check Results")
        dialog.setModal(True)
        dialog.resize(600, 400)
        
        layout = QVBoxLayout(dialog)
        
        label = QLabel("Found grammar issues:")
        layout.addWidget(label)
        
        list_widget = QListWidget()
        for error in corrections:
            item = QListWidgetItem(f"{error['error_type']}: {error['sentence']}\nSuggestion: {error['suggestion']}")
            list_widget.addItem(item)
        
        layout.addWidget(list_widget)
        
        button_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        button_box.accepted.connect(dialog.accept)
        layout.addWidget(button_box)
        
        dialog.exec()
        self.status_bar.showMessage(f"Grammar check completed - {len(corrections)} issues found")
    
    def check_spelling(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to check")
            return
        
        self.status_bar.showMessage("Checking spelling...")
        
        # Run in thread to avoid freezing GUI
        self.spelling_thread = WorkerThread(self.nlp_services.check_spelling, content)
        self.spelling_thread.finished.connect(self.spelling_check_finished)
        self.spelling_thread.error.connect(self.nlp_error)
        self.spelling_thread.start()
    
    def spelling_check_finished(self, corrections):
        if not corrections:
            QMessageBox.information(self, "Spelling Check", "No spelling errors found")
            self.status_bar.showMessage("Spelling check completed - no errors found")
            return
        
        # Highlight spelling errors in the text
        editor = self.get_current_editor()
        if editor:
            # Clear previous highlights
            cursor = editor.textCursor()
            cursor.select(QTextCursor.SelectionType.Document)
            format = QTextCharFormat()
            format.setUnderlineStyle(QTextCharFormat.UnderlineStyle.NoUnderline)
            cursor.mergeCharFormat(format)
            
            # Highlight new errors
            format.setUnderlineColor(QColor("red"))
            format.setUnderlineStyle(QTextCharFormat.UnderlineStyle.SpellCheckUnderline)
            
            for error in corrections:
                cursor.setPosition(error['start_pos'])
                cursor.setPosition(error['end_pos'], QTextCursor.MoveMode.KeepAnchor)
                cursor.mergeCharFormat(format)
        
        # Show spelling suggestions in a dialog
        dialog = QDialog(self)
        dialog.setWindowTitle("Spelling Check Results")
        dialog.setModal(True)
        dialog.resize(600, 400)
        
        layout = QVBoxLayout(dialog)
        
        label = QLabel("Found spelling errors:")
        layout.addWidget(label)
        
        list_widget = QListWidget()
        for error in corrections:
            suggestions = ", ".join(error['suggestions'])
            item = QListWidgetItem(f"'{error['word']}': Suggestions: {suggestions}")
            list_widget.addItem(item)
        
        layout.addWidget(list_widget)
        
        button_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        button_box.accepted.connect(dialog.accept)
        layout.addWidget(button_box)
        
        dialog.exec()
        self.status_bar.showMessage(f"Spelling check completed - {len(corrections)} errors found")
    
    def summarize_text(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to summarize")
            return
        
        self.status_bar.showMessage("Generating summary...")
        
        # Run in thread to avoid freezing GUI
        self.summary_thread = WorkerThread(self.nlp_services.summarize_text, content)
        self.summary_thread.finished.connect(self.summary_finished)
        self.summary_thread.error.connect(self.nlp_error)
        self.summary_thread.start()
    
    def summary_finished(self, summary):
        dialog = QDialog(self)
        dialog.setWindowTitle("Text Summary")
        dialog.setModal(True)
        dialog.resize(500, 400)
        
        layout = QVBoxLayout(dialog)
        
        label = QLabel("Summary:")
        layout.addWidget(label)
        
        text_edit = QPlainTextEdit()
        text_edit.setPlainText(summary)
        text_edit.setReadOnly(True)
        layout.addWidget(text_edit)
        
        button_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        button_box.accepted.connect(dialog.accept)
        layout.addWidget(button_box)
        
        dialog.exec()
        self.status_bar.showMessage("Summary generated")
    
    def calculate_readability(self):
        content = self.get_current_content()
        if not content:
            QMessageBox.information(self, "Info", "No content to analyze")
            return
        
        self.status_bar.showMessage("Calculating readability...")
        
        # Run in thread to avoid freezing GUI
        self.readability_thread = WorkerThread(self.nlp_services.calculate_readability, content)
        self.readability_thread.finished.connect(self.readability_finished)
        self.readability_thread.error.connect(self.nlp_error)
        self.readability_thread.start()
    
    def readability_finished(self, result):
        dialog = QDialog(self)
        dialog.setWindowTitle("Readability Score")
        dialog.setModal(True)
        dialog.resize(400, 300)
        
        layout = QVBoxLayout(dialog)
        
        label = QLabel(
            f"Flesch-Kincaid Readability Score: {result['score']:.2f}\n"
            f"Reading Level: {result['level']}\n\n"
            f"Statistics:\n"
            f"Sentences: {result['sentences']}\n"
            f"Words: {result['words']}\n"
            f"Syllables: {result['syllables']}"
        )
        layout.addWidget(label)
        
        button_box = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok)
        button_box.accepted.connect(dialog.accept)
        layout.addWidget(button_box)
        
        dialog.exec()
        self.status_bar.showMessage("Readability calculated")
    
    def nlp_error(self, error_message):
        QMessageBox.critical(self, "NLP Error", f"Error during NLP processing: {error_message}")
        self.status_bar.showMessage("NLP processing failed")
    
    def update_stats(self):
        stats = self.editor_core.get_performance_stats()
        stats_text = (
            f"Operations: {stats['operations']} | "
            f"Ops/Sec: {stats['ops_per_sec']:.2f} | "
            f"Tree Height: {stats['tree_height']} | "
            f"Balance: {stats['balance_factor']}"
        )
        self.status_bar.showMessage(stats_text)
        
        # Triggered by QTimer; do not create background threads here
    
    def update_status_bar(self):
        editor = self.get_current_editor()
        if editor:
            cursor = editor.textCursor()
            line = cursor.blockNumber() + 1
            column = cursor.columnNumber() + 1
            content = self.get_current_content()
            char_count = len(content)
            word_count = len(content.split())
            
            stats = self.editor_core.get_performance_stats()
            stats_text = (
                f"Line: {line}, Column: {column} | "
                f"Chars: {char_count}, Words: {word_count} | "
                f"Operations: {stats['operations']} | "
                f"Ops/Sec: {stats['ops_per_sec']:.2f}"
            )
            self.status_bar.showMessage(stats_text)
    
    def closeEvent(self, event):
        # Stop timers to avoid callbacks after destruction
        try:
            if hasattr(self, 'stats_timer') and self.stats_timer is not None:
                self.stats_timer.stop()
        except Exception:
            pass

        # Check for unsaved changes in all tabs
        unsaved_tabs = []
        for index in range(self.tab_widget.count()):
            if self.unsaved_changes.get(index, False):
                unsaved_tabs.append(index)
        
        if unsaved_tabs:
            reply = QMessageBox.question(
                self, "Unsaved Changes",
                "You have unsaved changes in some tabs. Do you want to save before exiting?",
                QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel
            )
            
            if reply == QMessageBox.StandardButton.Save:
                for index in unsaved_tabs:
                    self.tab_widget.setCurrentIndex(index)
                    if not self.save_file():
                        event.ignore()
                        return
            
            elif reply == QMessageBox.StandardButton.Cancel:
                event.ignore()
                return
        
        event.accept()

def main():
    app = QApplication(sys.argv)
    editor = TextEditor()
    editor.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()