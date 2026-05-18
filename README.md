# 📚 圖書管理系統（Library Management System）

> 國立陽明交通大學 電機工程學系（National Yang Ming Chiao Tung University, NYCU EE）  
> 物件導向程式設計（Object-Oriented Programming, OOP）期末專題（Final Project）

---

# 🧾 專案簡介（Introduction）

本專案為國立陽明交通大學電機工程學系（NYCU EE）物件導向程式設計（OOP）課程之期末專題。

本系統是一套以 C++ 開發、基於純物件導向架構（Object-Oriented Architecture）的終端機圖書管理系統（Terminal-based Library Management System）。系統將每一本書籍封裝（Encapsulation）為獨立物件（Object），透過繼承（Inheritance）、多型（Polymorphism) 等 OOP 技術，模擬真實圖書館中的搜尋（Searching）、管理（Management）與 預覽（Preview）功能。

不同於傳統純文字終端機程式（Text-only Terminal Program），本專案進一步整合：

- ASCII Art（字元畫）
- 動態數學公式解析（Dynamic Mathematical Expression Parsing）
- 視覺暫留動畫（Persistence of Vision Animation）
- 摩斯密碼翻譯（Morse Code Translation）
- 終端機頁面渲染系統（Terminal Rendering System）

讓 Linux 終端機（Linux Terminal）不再只是文字輸出，而能呈現具有互動性與視覺效果的體驗。

---

# ✨ 核心功能（Core Features）

## 1️⃣ 物件導向圖書管理架構（Object-Oriented Book Architecture）

- 將書籍資訊完整封裝於 Book 類別（Class）中
- 使用虛擬函式（Virtual Function）實現多型（Polymorphism）
- 利用動態綁定（Dynamic Binding）切換不同書籍型態
- 支援多型容器（Polymorphic Container）管理 heterogeneous objects

### 📖 支援書籍型態（Supported Book Types）

| 類型（Type） | 說明（Description） |
|---|---|
| T | 純文字書籍（TxtBook / Plain Text Book） |
| F | ASCII 字元畫書籍（FigBook / ASCII Figure Book） |
| M | 數學公式運算書籍（MthBook / Mathematical Book） |
| A | ASCII 動畫書籍（AniBook / ASCII Animation Book） |
| C | 摩斯密碼翻譯書籍（MorseBook / Morse Code Book） |

---

## 2️⃣ 多功能搜尋系統（Multi-Criteria Search System）

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

## 3️⃣ 進階電子書渲染技術（Advanced E-Book Rendering Technologies）

### 📐 MthBook — 動態數學公式解析（Dynamic Formula Engine）

系統可自動辨識（Parse）：

```txt
?5+3?
?sqrt(25)?
```

並於渲染（Rendering）時即時計算（Real-time Evaluation）結果後輸出。

---

### 🖼️ FigBook — ASCII 字元畫分頁系統（ASCII Figure Pagination System）

- 自動辨識圖片
- 防止 ASCII Art 被分頁（Pagination）切斷
- 自動將大型圖片（Large Figure）移動至下一頁（Next Page）

---

### 🎞️ AniBook — 終端機動畫系統（Terminal Animation System）

- 支援 ASCII 動畫（Multi-frame ASCII Animation）
- 流暢播放動畫效果（Smooth Animation Playback）

---

### 🔡 MorseBook — 摩斯密碼翻譯系統（Morse Code Translation System）

系統會自動將摩斯密碼（Morse Code）翻譯為英文與數字。

範例（Example）：

```txt
.... .
```

輸出（Output）：

```txt
HE
```

---

## 📊 借閱排行榜系統（Borrowing Statistics System）

- 即時統計 borrowCount（Borrow Count）
- 熱門書籍排行榜（Popularity Leaderboard）
- 動態更新借閱次數（Dynamic Statistics Update）

---

## ⚠️ 終端機邊界安全警示（Terminal Boundary Safety Warnings）

系統利用 ANSI Escape Code 顯示：

- 紅字警告（Red Warning Prompt）
- 邊界超出提示（Boundary Overflow Warning）
- 非法操作提醒（Invalid Operation Alert）

提升終端機使用者介面（Terminal UI）互動體驗。

---

# 📂 專案檔案結構（Project Structure）

```tree
├── main.cpp
├── library.cpp/.h
├── book.cpp/.h
├── page.cpp/.h
├── Makefile
└── TXT/ (書籍)
    ├── bookList.txt (書單)
    ├── book1.txt
    ├── image_book.txt
    ├── math_magic.txt
    ├── Anime.txt
    └── morse_sample.txt
```

---

# 🛠️ 開發技術（Tech Stack）

- 程式語言（Programming Language）：C++
- 開發環境（Development Environment）：Linux Terminal (wsl)

### 核心 OOP 概念（Core OOP Concepts）

- Encapsulation（封裝）
- Polymorphism（多型）
- Inheritance（繼承）

---

# 🌟 專案特色（Project Highlights）

- 純物件導向架構設計（Pure Object-Oriented Architecture）
- 多型電子書渲染系統（Polymorphic E-Book Rendering System）
- 動態 ASCII Art 引擎（Dynamic ASCII Rendering Engine）
- 終端機動畫播放器（Terminal Animation Player）
- 智慧分頁演算法（Smart Pagination Algorithm）
- 數學公式解析器（Mathematical Expression Parser）
- 摩斯密碼翻譯系統（Morse Code Translation System）
- Linux Terminal UI 互動設計（Interactive Terminal UI Design）

---

# 👨‍💻 開發者（Contributors）

- 張友銘（NYCU EE） - [GitHub Profile](https://github.com/dannlyee14-del)
- 陳暘暄（NYCU EE）

---