# expert_system

<img src="https://raw.githubusercontent.com/devicons/devicon/master/icons/cplusplus/cplusplus-original.svg" alt="cplusplus" width="50" height="50"/>

## 概要

このプロジェクトは42のカリキュラムの一部であり、与えられた事実とルールに基づいて推論できるエキスパートシステムを**C++**(言語は自由)で実装することを目的としています。このシステムは、命題論理演算子を用いた論理評価に従います。

## 内容

- 次の形式の入力ファイルから一連のルールと事実を解析します。

  ```
  C          =>  E
  A + B + C  =>  D
  A | B      =>  C
  A + !B     =>  F
  C | !G     =>  H
  V ^ W      =>  X
  A + B      =>  Y + Z
  C | D      =>  X | V
  E + F      =>  !V
  A          =>  !(B ^ C) + D
  A + B      <=> C

  =ABG  # Initial Facts

  ?GVX  # Queries
  ```

- Evaluates logical expressions using:

  - `!` (NOT)
  - `+` (AND)
  - `#` (OR)
  - `^` (XOR)
  - `(` and `)` for grouping expressions

- **AND**, **OR**, **XOR** および **結論における括弧**
- **二条件ルール:** 例えば, "`「AかつBはDの場合に限り成立する」など。`".
- 与えられたクエリの真理値を判定します。
- 矛盾を処理します。
- **インタラクティブな事実検証:** ユーザーは事実を変更することで、同じクエリを異なる入力に対して検証できます。
- **推論の​​視覚化:** 答えを説明するフィードバックを提供します。例えば、"`「Aは真であることが分かっています。A | B => Cが分かっているので、Cは真です」`" など。

##　ビルド手順
```bash
git clone https://github.com/KWEKE633/expert_system.git

make

./expert_system example_input.txt
```
