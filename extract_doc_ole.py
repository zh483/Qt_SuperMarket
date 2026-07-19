import olefile
import sys
import re

def extract_doc_text(path):
    ole = olefile.OleFileIO(path)
    if not ole.exists('WordDocument'):
        print("No WordDocument stream found")
        return
    stream = ole.openstream('WordDocument').read()
    ole.close()
    
    # Try decoding as UTF-16LE
    # Text usually starts around offset 0x400, but let's try whole stream
    texts = []
    for encoding in ['utf-16le', 'gbk', 'gb2312', 'big5']:
        try:
            text = stream.decode(encoding, errors='ignore')
            # Filter printable characters and Chinese
            text = re.sub(r'[\x00-\x08\x0b-\x0c\x0e-\x1f]', '', text)
            texts.append((encoding, text))
        except Exception as e:
            pass
    
    # Pick the one with most Chinese characters
    best = max(texts, key=lambda x: sum(1 for c in x[1] if '\u4e00' <= c <= '\u9fff'))
    print(f"Best encoding: {best[0]}")
    return best[1]

if __name__ == '__main__':
    text = extract_doc_text('需求分析和数据库设计格式参考2026.doc')
    if text:
        with open('reference_doc_ole.txt', 'w', encoding='utf-8') as f:
            f.write(text)
        print(f"Extracted {len(text)} chars")
