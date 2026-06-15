# 📚 圖書管理系統（Library Management System）

> 國立陽明交通大學 電機工程學系（National Yang Ming Chiao Tung University, NYCU EE）  
> 物件導向程式設計（Object-Oriented Programming, OOP）期末專題（Final Project）

---

# 🧾 專案簡介（Introduction）

本專案為國立陽明交通大學電機工程學系（NYCU EE）物件導向程式設計（OOP）課程之期末專題。

本系統是一套以 C++ 開發、基於物件導向架構（Object-Oriented Architecture）的圖書管理系統）。系統將每一本書籍封裝（Encapsulation）為獨立物件（Object），透過繼承（Inheritance）、多型（Polymorphism) 等 OOP 技術，模擬真實圖書館中的搜尋（Searching）、管理（Management）與 預覽（Preview）功能。

不同於傳統純文字終端機程式（Text-only Terminal Program），本專案進一步整合：

- ASCII Art（字元畫）
- 動態數學公式解析（Dynamic Mathematical Expression Parsing）
- 視覺暫留動畫（Persistence of Vision Animation）
- 摩斯密碼翻譯（Morse Code Translation）
- 終端機頁面渲染系統（Terminal Rendering System）

---

# ✨ 核心功能（Core Features）

### 📖 支援書籍型態（Supported Book Types）

| 類型（Type） | 說明（Description） |
|---|---|
| T | 純文字書籍（TxtBook / Plain Text Book） |
| F | ASCII 字元畫書籍（FigBook / ASCII Figure Book） |
| M | 數學公式運算書籍（MthBook / Mathematical Book） |
| A | ASCII 動畫書籍（AniBook / ASCII Animation Book） |
| C | 摩斯密碼翻譯書籍（MorseBook / Morse Code Book） |

---

支援以下搜尋方式（Search Criteria）：

- 檔案名稱（Filename）
- 書名（Title）
- 作者（Author）
- 類別（Category）
- 全文關鍵字搜尋（Full-text Content Search）

額外功能（Additional Features）：

- 即時新增書籍（Real-time Book Insertion）
- 自動建立檔案（Automatic File Creation）
- 動態更新藏書列表（Dynamic Library Update）

---

# 📂 專案檔案結構（Project Structure）

```tree
├── main.cpp
├── library.cpp/.h
├── book.cpp/.h
├── page.cpp/.h
├── Makefile
├── Database/ (系統資料)
│   ├── bookList.txt (書單)
│   ├── reviews.txt
│   ├── borrow.txt
│   └── alive.txt
├── UserData/ (使用者資料)
│   └── users.txt
└── Bookshelf/ (書籍內容)
    ├── book1.txt
    ├── image_book.txt
    ├── math_magic.txt
    ├── Anime.txt
    └── morse_sample.txt
```
# 作者
- 張友銘（NYCU EE） - [GitHub Profile](https://github.com/dannlyee14-del)
- 陳暘暄（NYCU EE）
---
