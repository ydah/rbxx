# ADR 0001: Test extension module name

- 背景: 作業指示は `rbxxTest` を指定するが、CRuby の定数名は大文字で始める必要がある。
- 選択肢: 無効な名前を強制する / 有効な `RbxxTest` に統一する。
- 決定: C++ と Ruby のテスト拡張名前空間を `RbxxTest` に統一する。
- 影響: 指示書の表記と大文字小文字だけ異なるが、全対応 OS/Ruby で正しくロードできる。
