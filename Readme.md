
# rplsTOOL

## このリポジトリについて

このリポジトリは、2018年8月頃まで Vesti La Giubba [http://saysaysay.net/rplstool](https://web.archive.org/web/20180309090645/http://saysaysay.net/rplstool) にて公開されていた、  
rplsTOOL v2.10 ([Wayback Machine](https://web.archive.org/web/20180309090645/http://saysaysay.net/tools/rplstool210.zip)) に同梱されているソースコードのミラーです。  

UI の各種改善を行ったこと、ファイル選択ダイヤログで MPEG2-TS を選択できるようにしたこと、ビルド環境を Visual Studio 2019 (VS2019) に更新したこと、rplstool.sln（ソリューションファイル）を作成したこと、Readme.txt を現状に合わせて改変したこと、この Readme.md を作成したこと以外はオリジナルのままとなっています。  
このリポジトリは VS2019 に更新されていますが、Release の [v2.10](https://github.com/tsukumijima/rplsTOOL/releases/tag/v2.10) (rplsTOOL210.zip) に関してはオリジナルのもの（再配布）です。  
Release の [v2.11](https://github.com/tsukumijima/rplsTOOL/releases/tag/v2.11) (rplsTOOL211.zip) は UI の各種改善を行い、ファイル選択ダイヤログで MPEG2-TS を選択できるようにし、ビルド環境を VS2019 に更新したものです。v2.10 よりも使いやすくなっていると思います。

以下のドキュメントは [http://saysaysay.net/rplstool](https://web.archive.org/web/20180309090645/http://saysaysay.net/rplstool) に記載されていた情報を現状に合わせて一部改変し、Markdown 形式に書き換えたものです。  
[Readme.txt](Readme.txt)・[ソースファイルについて.txt](ソースファイルについて.txt) も参照してください。

元々 rpls ファイルを編集するためのツールですが、録画した TS ファイルの番組情報を確認する用途でも使いやすいツールです。  
rplsTOOL の作者の方に感謝します。

----

rplsTOOL は、rpls ファイルを編集・修正するツールです。  
おまけの機能として TS ファイル中の番組情報を表示する機能も有しています。

このツールの使用は使用者の自己責任において行って下さい。  
このツールの使用によってもたらされた如何なる結果にも作者は一切責任を負いません。

「 rpls ファイル」が何なのかがよく分からない方は、[こちらの解説](https://web.archive.org/web/20170111210230/http://saysaysay.net/archives/124)を読んで下さい。

rpls ファイルは必ずバックアップしましょう。  
編集後、何らかの原因で BDAV ディスクが再生できなくなった場合は、オリジナルの rpls ファイルに戻して下さい。

- [rplsTOOL](#rplstool)
  - [このリポジトリについて](#このリポジトリについて)
  - [ダウンロード](#ダウンロード)
    - [アーカイブの内容](#アーカイブの内容)
  - [動作環境](#動作環境)
  - [アプリケーションの使用方法](#アプリケーションの使用方法)
  - [動作オプション指定について](#動作オプション指定について)
  - [番組情報のクリップボードへのコピー](#番組情報のクリップボードへのコピー)
    - [コピーメニューの一覧](#コピーメニューの一覧)
  - [TSファイルの番組情報表示機能について](#tsファイルの番組情報表示機能について)
  - [「追加記号」と「追加漢字」について](#追加記号と追加漢字について)
  - [注意点](#注意点)
  - [再配布について](#再配布について)
  - [更新履歴](#更新履歴)

## ダウンロード

[rplsTOOL Version 2.11](https://github.com/tsukumijima/rplsTOOL/releases/download/v2.11/rplsTOOL211.zip)

[rplsTOOL Version 2.10](https://github.com/tsukumijima/rplsTOOL/releases/download/v2.10/rplsTOOL210.zip)

### アーカイブの内容

* Readme.txt … 説明ファイル
* rplsTOOL.exe … アプリケーション本体
* x64 フォルダ … 64bit版アプリケーション本体
* src フォルダ … アプリケーションのソースファイル（ Version 2.10 のみ）

## 動作環境

本ツールは主に Windows 10 で動作を確認しています。
Version 2.11 から開発環境が Visual Studio 2019 になりました。

実行環境によっては適切なランタイムパッケージのインストールが必要になります。  
エラーが出てアプリケーションを起動できない場合は、マイクロソフト社のページから以下のパッケージをダウンロード、インストールして下さい。  
32bit 版、64bit 版アプリケーションの実行に必要なランタイムパッケージは、それぞれ以下のものです。  

* [Microsoft Visual C++ 2015-2019 再頒布可能パッケージ (x86)](https://aka.ms/vs/16/release/VC_redist.x86.exe)  
* [Microsoft Visual C++ 2015-2019 再頒布可能パッケージ (x64)](https://aka.ms/vs/16/release/VC_redist.x64.exe)

## アプリケーションの使用方法

アプリケーションアイコンをダブルクリックして実行して下さい。  
「ファイルを参照」ボタンを押して rpls ファイル・TS ファイルを選択するか、開いたウインドウ上に rpls ファイル・TS ファイルをドラッグ＆ドロップして読み込みます。  
アプリケーションアイコン、もしくはアプリケーションウインドウに複数のファイルをドラッグ＆ドロップすると、個数分のアプリケーションが起動されます。

番組名等の内容を変更した後、「保存」ボタンを押すとファイルに変更内容を保存し、アプリケーションは初期化された状態に戻ります。  
Shift キーを押した状態で「保存」ボタンを押すと、ファイルに変更内容を保存した後、アプリケーションはすぐ終了します。

「元に戻す」ボタンを押すと、ファイル読み込み直後の未変更状態に戻ります。「終了」ボタンを押すと、変更内容を保存せずにアプリケーションを終了します。

rpls ファイルを書き込み可能な状態で開くことが出来なかった場合は、読み取り専用で　rpls　ファイルを開きます。  
その場合は保存出来ませんので、保存ボタンはグレーアウトして使えない状態になります。TS ファイルを開いている場合も同様です。

## 動作オプション指定について

rplsTOOL.ini ファイルを編集することによって、アプリケーションの動作選択オプションを指定することが出来ます。  
ini ファイルが存在しない場合、アプリケーションを一度起動、終了すると自動で作成されます。

オプションの指定は以下の通りです。

* **APPLYCHARSIZE=0** もしくは **1** (デフォルト=0)
  - 1を指定すると番組情報読み込み時に、英数文字に対して文字サイズ指定 (NSZ, MSZ) を反映させます。

* **SAVECHARSIZE=0** もしくは **1** (デフォルト=1)
  - 0を指定すると編集した番組内容等の保存時に、文字サイズ指定 (NSZ, MSZ) を含めない様にします。
  - 編集時に英数文字等の半角文字を入力しても、レコーダ上での表示は全角表示になります。
  - 0の指定時は、アプリケーションウインドウのタイトルに ”*” が表示されます。

* **USEIVS=0** もしくは **1** (デフォルト=0)
  - 1を指定すると、複数の異なる字体が存在する文字 (葛, 辻, 祇など) の出力時、その区別に異体字セレクタを使用する．

* **FORCENORMALRPLS=0** もしくは **1** (デフォルト=0)
  - 1を指定すると強制的に非ソニーレコ製 rpls 編集モードで起動します

* **QUICKEXIT=0** もしくは **1** (デフォルト=0)
  - 1を指定すると保存ボタンを押した際、変更内容を保存した後すぐアプリケーションを終了します。
  - Shift キー＋保存ボタンと同じ動作です。

* **SHOWMAX=0** もしくは **1** (デフォルト=0)
  - 1を指定するとアプリケーションウインドウを最大化した状態で起動します。

* **TSFILEPOS=整数値** (デフォルト=50)
  - 0～99の数値を指定し、TS ファイル内からの番組情報取得位置を設定します。0が TS ファイルの先頭、99がファイルの終端を示しています。

* **PACKETLIMIT=整数値** (デフォルト=0)
  - TS ファイルからの番組情報パケットの探索リミットを整数値で指定します。必要な番組情報がまだ見つからない場合でも ”指定した数値MByte” 分まで進行した所で探索を終了します。デフォルトの 0 では、ファイルの終端まで番組情報の探索を行います。

* **FONTNAME=フォント名** (デフォルト=Meiryo UI)
  - 編集画面の表示フォントを指定します。メイリオやその他、日本語表示が可能なフォントを指定することが可能です。

* **FONTSIZE=整数値** (デフォルト=18)
  - 編集画面の表示フォントのサイズを指定します。大きなフォントサイズを指定すると表示領域をはみ出す場合がありますが、動作に支障はありません。

SAVECHARSIZE=0 を指定すると、文字サイズ指定が省略される分データが短くなりますので、放送局名などがデータの長さ制限にかかって入力できない場合に使用して下さい。（英数等の半角文字と、漢字等の全角文字が混在するとデータが長くなります）

## 番組情報のクリップボードへのコピー

アプリケーションウインドウ上で右クリックすると、コピーメニューが表示されます。  
メニュー上のチェックされた番組情報項目が、指定形式でクリップボードにコピー出力されます。

### コピーメニューの一覧

* テキスト形式コピー … 各項目を、通常のテキスト形式で出力します。各項目は改行で区切られます。
* 項目名付テキスト形式コピー … 各項目を、[項目名]を含むテキスト形式で出力します。各項目は改行で区切られます。
* CSV 形式コピー … 各項目を CSV 形式で出力します。各項目の前後は「”」で囲まれ、項目中の「”」は「”」でエスケープされます。区切り文字は半角コンマ「,」です。

コピーに必要な情報項目をチェックし、不要な項目はチェックをはずして使用して下さい。

## TSファイルの番組情報表示機能について

アプリケーションから TS ファイルを開くと、ファイル内の番組情報を表示することが出来ます。但し読込表示のみで、編集保存は出来ません。

番組情報は通常 TS ファイルの中央付近から取得されますが、ini ファイル中の ”TSFILEPOS” を変更する事で取得位置を変更することが出来ます。  
0～99 の数値で指定し、0 がファイルの先頭を示し、99 がファイルの終端です。範囲外の指定は無視されます。デフォルト値 50 です。このオプション指定は rpls ファイルの編集には影響を及ぼしません。

## 「追加記号」と「追加漢字」について

番組名や番組内容に含まれる特殊な文字（追加記号と呼ばれるもの）の内、よく使われるものについては、本アプリケーション内では以下のリストのように扱います。

* 90区48点から63点までの追加記号 … [HV] [SD] [P] [W] [MV] [手] [字] [双] [デ] [S] [二] [多] [解] [SS] [B] [N]
* 90区66点から84点までの追加記号 … [天] [交] [映] [無] [料] [鍵] [前] [後] [再] [新] [初] [終] [生] [販] [声] [吹] [PPV] [秘] [ほか]

追加記号の内、Windows 側に対応する文字のあるものは直接扱えます。主に「① ② ③」のような丸囲み数字や、「Ⅰ Ⅱ Ⅲ」などのローマ数字です。

これら以外の、直接文字として扱えない追加記号については、[#XX#YY] という形で扱います。XX、YY は追加記号の区点番号です。  
区点番号については ARIB STD-B24 などを参照して下さい。対応する Unicode 文字があるものについてはおよそ扱えます。

一部の特別な漢字は「追加漢字」として、追加記号の85区1点から86区43点に割り当てられています。これらは追加記号と同様の扱いになります。例えば人名等に用いられる事のある「髙（はしごだか）」や「﨑（たちざき）」などは対応する文字があるので直接扱えます。直接扱えない文字については追加記号と同様です。

## 注意点

本ツールは読み込んだ元ファイルを直接書き換えるアプリケーションです。意図した結果にならなかった場合に備えて、必ずバックアップしたファイルを扱って下さい。

ソニー製レコーダで作成したrplsファイルでは、番組詳細情報、番組ジャンルを扱うことができます。ソニー製であってもrplsファイルに番組ジャンル情報が無い場合（アナログライン入力録画）や、互換性の無い番組ジャンル情報である場合（アナログ地上波放送録画）は、番組ジャンル表示が空欄になります。もしこれを書き換えたくない場合は空欄のままにしてください。番組ジャンルを指定すると、互換性のある番組ジャンル情報に書き換えます。

ソニー製レコーダ BDZ-X90 では、番組タイトルや番組内容等に JIS X0201 片仮名（いわゆる半角カタカナ）を使用することが出来ますが、PS3 や他社製レコーダとの互換性に問題があるようなので、半角カタカナの使用はお勧めしません。

パナ製レコーダで作成した rpls ファイルでは、放送種別情報（BSデジタル, 地上デジタル, i.LINK等）を扱うことが可能です。

Version 1.8 まではアプリケーションがソニー製 rpls 用、非ソニー製 rpls 用に分かれていましたが、Version 2.0 からはひとつのアプリケーションで動作を切り替える仕様になりました。

## 再配布について

本ツールの再配布に制限はありませんので、自由に再配布してくださって構いません。  
ただしツールの再配布によってもたらされた如何なる結果にも作者は一切責任を負いません。再配布に際して作者への連絡・許可は必要ありません。

ソースを改変したオリジナルバージョン等を公開される場合にも、作者への連絡･許可は必要ありません。## 更新履歴

## 更新履歴

Version | 更新内容
---- | ----
2.11 | ダイヤログの UI を調整しました。<br>ダイヤログの背景色を白に変更しました。<br>ダイヤログのフォントを Meiryo UI に変更しました。<br>フォントのデフォルトサイズを 18px に拡大しました。<br>ファイル選択ダイヤログで MPEG2-TS (.ts,.mts,.m2t,.m2ts) を選択できるようにしました。<br>開発環境を Visual Studio 2019 に変更しました。
2.10 | 文字コード変換ルーチンを更新しました。
2.9 | 文字コード変換ルーチンを更新しました。<br>編集中のファイル名をウインドウタイトルバーに表示するようにしました。<br>編集画面の表示フォントとそのサイズを指定できるようにしました。<br>異体字セレクタの使用オプションを追加しました。<br>TS ファイルからの番組情報パケットの探索リミットを設定するオプションを追加しました。<br>アプリケーションの起動時、及び D&D でファイルを開く際、複数ファイルを一度に開けるようにしました。
2.8 | 複数の番組ジャンル情報編集に対応しました。
2.7 | アプリケーションウインドウの表示位置･サイズ情報を保存する仕様になりました。<br>コマンドライン起動オプションを廃止し、.iniファイルによる設定に変更しました。
2.6 | 文字コード変換ルーチンの不具合を修正しました。
2.5 | パナ製レコーダ作成の rpls ファイルでの放送種別情報表示に対応しました。
2.4 | パナ製レコーダ作成の rpls ファイルでのジャンル表示に対応しました。<br>保存時の改行コードの仕様を変更しました。
2.3 | クリップボードへの番組情報のコピー出力機能を追加しました。<br>-y オプションを追加しました。
2.2 | マルチセクションパケットを有するTSからの番組情報取得を改善しました。
2.1 | アプリケーションウインドウをリサイズ可能にしました。<br>TS ファイル内の番組情報表示に対応しました。
2.0 | アプリケーションの動作仕様を変更しました。-n オプションを追加しました。
1.8 | 起動時のオプション指定を付けました。
1.7 | アプリケーションの終了方法を一部変更しました。
1.6 | 改行コードの扱いに関する不具合を修正しました。<br>番組ジャンル情報の扱いを一部変更しました。
1.5 | 追加漢字を変換できない不具合を修正しました。
1.4 | 読み取り専用ファイルも開けるように変更しました。<br>未編集時には終了警告を出さないようにしました。
1.3 | 放送局名に半角英数を表示できるように変更しました。<br>半角カタカナを入力した際、レコーダ側で全角表示されてしまう問題を修正しました。
1.2 | 画面 DPI 設定変更時にウインドウ内のレイアウトが乱れる問題を修正しました。<br>アプリケーション起動時のファイル名指定読み込みに対応しました。
1.1 | 追加記号の扱いを一部変更しました。<br>ソニー製レコーダ作成の rpls ファイルに対応しました。
1.0 | 最初のバージョン。
