import os
import json
import heapq
from collections import deque
from cryptography.fernet import Fernet
import time
import threading
import re

class AVLNode:
    """Node for AVL Tree that stores text content"""
    def __init__(self, key, text):
        self.key = key  # Position in the text
        self.text = text  # Text content at this position
        self.left = None
        self.right = None
        self.height = 1
        self.size = len(text)  # Size of subtree (total characters)

class AVLTree:
    """AVL Tree for storing text content efficiently"""
    def __init__(self):
        self.root = None
        self.operation_count = 0
        self.start_time = time.time()
    
    def get_height(self, node):
        if not node:
            return 0
        return node.height
    
    def get_balance(self, node):
        if not node:
            return 0
        return self.get_height(node.left) - self.get_height(node.right)
    
    def get_size(self, node):
        if not node:
            return 0
        return node.size
    
    def update_stats(self):
        self.operation_count += 1
    
    def get_stats(self):
        elapsed = time.time() - self.start_time
        ops_per_sec = self.operation_count / elapsed if elapsed > 0 else 0
        return {
            "operations": self.operation_count,
            "ops_per_sec": ops_per_sec,
            "tree_height": self.get_height(self.root),
            "balance_factor": self.get_balance(self.root) if self.root else 0
        }
    
    def insert(self, key, text):
        self.root = self._insert(self.root, key, text)
        self.update_stats()
    
    def _insert(self, node, key, text):
        if not node:
            return AVLNode(key, text)
        
        if key < node.key:
            node.left = self._insert(node.left, key, text)
        else:
            node.right = self._insert(node.right, key, text)
        
        node.height = 1 + max(self.get_height(node.left), self.get_height(node.right))
        node.size = len(node.text) + self.get_size(node.left) + self.get_size(node.right)
        
        balance = self.get_balance(node)
        
        # Left Left Case
        if balance > 1 and key < node.left.key:
            return self.right_rotate(node)
        
        # Right Right Case
        if balance < -1 and key > node.right.key:
            return self.left_rotate(node)
        
        # Left Right Case
        if balance > 1 and key > node.left.key:
            node.left = self.left_rotate(node.left)
            return self.right_rotate(node)
        
        # Right Left Case
        if balance < -1 and key < node.right.key:
            node.right = self.right_rotate(node.right)
            return self.left_rotate(node)
        
        return node
    
    def left_rotate(self, z):
        y = z.right
        T2 = y.left
        
        y.left = z
        z.right = T2
        
        z.height = 1 + max(self.get_height(z.left), self.get_height(z.right))
        y.height = 1 + max(self.get_height(y.left), self.get_height(y.right))
        
        z.size = len(z.text) + self.get_size(z.left) + self.get_size(z.right)
        y.size = len(y.text) + self.get_size(y.left) + self.get_size(y.right)
        
        return y
    
    def right_rotate(self, z):
        y = z.left
        T3 = y.right
        
        y.right = z
        z.left = T3
        
        z.height = 1 + max(self.get_height(z.left), self.get_height(z.right))
        y.height = 1 + max(self.get_height(y.left), self.get_height(y.right))
        
        z.size = len(z.text) + self.get_size(z.left) + self.get_size(z.right)
        y.size = len(y.text) + self.get_size(y.left) + self.get_size(y.right)
        
        return y
    
    def search(self, key):
        return self._search(self.root, key)
    
    def _search(self, node, key):
        if not node:
            return None
        
        if node.key == key:
            return node.text
        
        if key < node.key:
            return self._search(node.left, key)
        
        return self._search(node.right, key)
    
    def in_order_traversal(self):
        result = []
        self._in_order_traversal(self.root, result)
        return result
    
    def _in_order_traversal(self, node, result):
        if node:
            self._in_order_traversal(node.left, result)
            result.append((node.key, node.text))
            self._in_order_traversal(node.right, result)
    
    def get_text(self):
        nodes = self.in_order_traversal()
        nodes.sort(key=lambda x: x[0])
        return "".join(text for _, text in nodes)
    
    def delete(self, key):
        self.root = self._delete(self.root, key)
        self.update_stats()
    
    def _delete(self, root, key):
        if not root:
            return root
        
        if key < root.key:
            root.left = self._delete(root.left, key)
        elif key > root.key:
            root.right = self._delete(root.right, key)
        else:
            if root.left is None:
                return root.right
            elif root.right is None:
                return root.left
            
            temp = self.get_min_value_node(root.right)
            root.key = temp.key
            root.text = temp.text
            root.right = self._delete(root.right, temp.key)
        
        if root is None:
            return root
        
        root.height = 1 + max(self.get_height(root.left), self.get_height(root.right))
        root.size = len(root.text) + self.get_size(root.left) + self.get_size(root.right)
        
        balance = self.get_balance(root)
        
        # Left Left Case
        if balance > 1 and self.get_balance(root.left) >= 0:
            return self.right_rotate(root)
        
        # Left Right Case
        if balance > 1 and self.get_balance(root.left) < 0:
            root.left = self.left_rotate(root.left)
            return self.right_rotate(root)
        
        # Right Right Case
        if balance < -1 and self.get_balance(root.right) <= 0:
            return self.left_rotate(root)
        
        # Right Left Case
        if balance < -1 and self.get_balance(root.right) > 0:
            root.right = self.right_rotate(root.right)
            return self.left_rotate(root)
        
        return root
    
    def get_min_value_node(self, root):
        if root is None or root.left is None:
            return root
        return self.get_min_value_node(root.left)

class EditorCore:
    """Core editor functionality with AVL tree, undo/redo, file operations, encryption, and compression"""
    def __init__(self):
        self.avl_tree = AVLTree()
        self.undo_stack = deque(maxlen=100)
        self.redo_stack = deque(maxlen=100)
        self.current_position = 0
        self.encryption_key = None
    
    def insert_text(self, position, text):
        # Split the text into chunks for efficient storage
        chunk_size = 100  # characters per node
        for i in range(0, len(text), chunk_size):
            chunk = text[i:i+chunk_size]
            self.avl_tree.insert(position + i, chunk)
        
        self.undo_stack.append(('insert', position, text))
        self.redo_stack.clear()
        self.current_position = position + len(text)
    
    def delete_text(self, position, length):
        # Find and remove nodes in the range
        nodes = self.avl_tree.in_order_traversal()
        for key, text in nodes:
            if position <= key < position + length:
                self.avl_tree.delete(key)
            elif key >= position + length:
                # Adjust positions of subsequent nodes
                self.avl_tree.delete(key)
                self.avl_tree.insert(key - length, text)
        
        self.undo_stack.append(('delete', position, length))
        self.redo_stack.clear()
    
    def undo(self):
        if not self.undo_stack:
            return False
        
        action = self.undo_stack.pop()
        self.redo_stack.append(action)
        
        if action[0] == 'insert':
            self.delete_text(action[1], len(action[2]))
        elif action[0] == 'delete':
            self.insert_text(action[1], action[2])
        
        return True
    
    def redo(self):
        if not self.redo_stack:
            return False
        
        action = self.redo_stack.pop()
        self.undo_stack.append(action)
        
        if action[0] == 'insert':
            self.insert_text(action[1], action[2])
        elif action[0] == 'delete':
            self.delete_text(action[1], action[2])
        
        return True
    
    def open_file(self, file_path):
        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                content = file.read()
            
            # Rebuild AVL tree from file content
            self.avl_tree = AVLTree()
            self.insert_text(0, content)
            self.undo_stack.clear()
            self.redo_stack.clear()
            self.current_position = len(content)
            
            return True, "File opened successfully"
        except Exception as e:
            return False, f"Error opening file: {str(e)}"
    
    def save_file(self, file_path, content):
        try:
            with open(file_path, 'w', encoding='utf-8') as file:
                file.write(content)
            return True, "File saved successfully"
        except Exception as e:
            return False, f"Error saving file: {str(e)}"
    
    def encrypt_file(self, content, key=None):
        try:
            # Generate or normalize key to bytes
            if key is None:
                key_bytes = Fernet.generate_key()
            else:
                key_bytes = key.encode() if isinstance(key, str) else key

            fernet = Fernet(key_bytes)
            encrypted_content = fernet.encrypt(content.encode())
            self.encryption_key = key_bytes
            return True, encrypted_content, key_bytes
        except Exception as e:
            return False, f"Error encrypting file: {str(e)}", None
    
    def decrypt_file(self, encrypted_content, key):
        try:
            key_bytes = key.encode() if isinstance(key, str) else key
            fernet = Fernet(key_bytes)
            decrypted_content = fernet.decrypt(encrypted_content).decode()
            return True, decrypted_content
        except Exception as e:
            return False, f"Error decrypting file: {str(e)}"
    
    def compress_content(self, content):
        """Huffman compression implementation"""
        try:
            frequency = {}
            for char in content:
                frequency[char] = frequency.get(char, 0) + 1
            
            heap = [[weight, [char, ""]] for char, weight in frequency.items()]
            heapq.heapify(heap)
            
            while len(heap) > 1:
                lo = heapq.heappop(heap)
                hi = heapq.heappop(heap)
                for pair in lo[1:]:
                    pair[1] = '0' + pair[1]
                for pair in hi[1:]:
                    pair[1] = '1' + pair[1]
                heapq.heappush(heap, [lo[0] + hi[0]] + lo[1:] + hi[1:])
            
            huffman_codes = sorted(heapq.heappop(heap)[1:], key=lambda p: (len(p[-1]), p))
            
            encoding_table = {char: code for char, code in huffman_codes}
            
            compressed_content = ''.join(encoding_table[char] for char in content)
            
            # Convert binary string to bytes
            extra_bits = 8 - (len(compressed_content) % 8)
            compressed_content += '0' * extra_bits
            
            compressed_bytes = bytearray()
            for i in range(0, len(compressed_content), 8):
                byte = compressed_content[i:i+8]
                compressed_bytes.append(int(byte, 2))
            
            metadata = {
                'encoding_table': encoding_table,
                'extra_bits': extra_bits
            }
            
            return True, bytes(compressed_bytes), metadata
        except Exception as e:
            return False, f"Error compressing content: {str(e)}", None
    
    def decompress_content(self, compressed_bytes, metadata):
        """Huffman decompression implementation"""
        try:
            encoding_table = metadata['encoding_table']
            extra_bits = metadata['extra_bits']
            
            # Reverse the encoding table
            decoding_table = {code: char for char, code in encoding_table.items()}
            
            # Convert bytes to binary string
            binary_string = ''.join(f'{byte:08b}' for byte in compressed_bytes)
            
            # Remove extra bits added during compression
            if extra_bits > 0:
                binary_string = binary_string[:-extra_bits]
            
            # Decode the binary string
            current_code = ""
            decompressed_content = []
            
            for bit in binary_string:
                current_code += bit
                if current_code in decoding_table:
                    decompressed_content.append(decoding_table[current_code])
                    current_code = ""
            
            return True, ''.join(decompressed_content)
        except Exception as e:
            return False, f"Error decompressing content: {str(e)}"
    
    def get_performance_stats(self):
        return self.avl_tree.get_stats()