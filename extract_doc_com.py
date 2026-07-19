import win32com.client
import os
import sys

word = win32com.client.Dispatch("Word.Application")
word.Visible = False

doc_path = os.path.join(os.getcwd(), "需求分析和数据库设计格式参考2026.doc")
out_path = os.path.join(os.getcwd(), "reference_doc_com.txt")

try:
    doc = word.Documents.Open(doc_path)
    text = doc.Content.Text
    doc.Close()
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(text)
    print(f"Extracted {len(text)} chars to {out_path}")
except Exception as e:
    print(f"Error: {e}")
finally:
    word.Quit()
