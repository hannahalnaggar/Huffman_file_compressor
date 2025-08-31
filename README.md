# Huffman Coding in C++

This project is an implementation of **Huffman Coding** for text compression using C++.

## 📌 Features
- Count character frequencies in a given input
- Build a Huffman tree using a priority queue
- Generate binary codes for each character
- Encode input text into compressed form
- Decode compressed text back to original

## ⚙️ How It Works
1. The program calculates the frequency of each character in the input.
2. It builds a Huffman Tree by combining the least frequent characters step by step.
3. Each character is assigned a binary code (shorter codes for more frequent characters).
4. The input string is encoded using these codes.
5. The encoded string can then be decoded using the Huffman Tree.

## 🚀 How to Run
1. Clone this repository:
   ```bash
   git clone https://github.com/your-username/Huffman-Coding.git
   cd Huffman-Coding
