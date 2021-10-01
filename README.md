# RecottePlugin

[![MSBuild](https://github.com/wallstudio/RecottePlugin/actions/workflows/msbuild.yml/badge.svg)](https://github.com/wallstudio/RecottePlugin/actions/workflows/msbuild.yml)

RecotteStudioのプラグインです。

![](img/ss1.gif)

## 対応バージョン

- RecotteStudio v1.4.0.0
- RecotteStudio v1.3.7.6

## インストール方法

1. RecotteStudioをインストール

1. [こちら](https://github.com/wallstudio/RecottePlugin/releases/)から最新版をダウンロードして適当なところに展開してください。

1. `RecottePlugin`フォルダをUserフォルダの中に配置します。

1. `RecottePlugin\install.bat`をダブルクリックで実行し、RecotteStudioのインストールフォルダに`d3d11.dll`がインストールされます。

1. レコスタを起動すると、`RecottePlugin`フォルダ内のプラグインが読み込まれます。

```
Cドライブ
  └ Users
    └ <ユーザー名>
      └ RecottePlugin
        ├ README.md
        ├ install.bat
        ├ RecottePluginManager.dll
        └ …
```

#### アップデート

`RecottePlugin\install.bat` に古いバージョンのアンインストールも含まれているので、`RecottePlugin`フォルダを丸ごと取り換えて `install.bat` を実行するだけです。

#### アンインストール

`C:\Program Files\RecotteStudio\d3d11.dll` のリンクファイルを削除してください。

## 機能

#### RecottePluginManager

PluginをロードするためのPluginです。

`RecottePluginManager.dll`は`install.bat`によって`d3d11.dll`としてRecotteStudioのインストールフォルダにインストールされます。
シンボリックリンクの形としてインストールされるので元のファイルも削除しないでください。

（これは`C:\Windows\System32\d3d11.dll`のProxyDLLとして動作します）

#### ~~LayerFolding~~

[公式で実装されました！🎉🎉🎉](https://twitter.com/ahsoft/status/1443096468257062914
)

それに伴い、RecottePluginからは削除しました。

#### CustomSkin

タイムラインの背景に画像を描画する機能です。

`RecottePlugin`フォルダ内に`skin.png`の名前で画像を入れておくことでタイムラインに召喚することができます。  

デフォルトで私服のマキさんが描かれるようになっています。ﾏｷﾏｷｶﾜｲｲﾔｯﾀｰ！

#### MultiEncodeTextReader 

RecotteStudioが`*.txt`ファイルを読み込む際に、本来であれば`shift-jis`でないと読み込めないところを他の文字コードでも読み込めるようにします。

ファイルの中身から自動判定をしている都合、ファイル内の文字数が1、2文字の場合はうまくいかないかもしれません。対応している文字コードは[`MLang`に準じ](https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/aa767865(v=vs.85))ます

#### RecotteShaderLoader

[RecotteShader](https://github.com/wallstudio/RecotteShader)を読み込む機能です。
詳細はRecotteShader側のドキュメントを参照してください。

<!-- 
#### ExternalPreviewView

プレビュー画面を外部Windowにします。
-->

## 不具合報告

issuesまたは、[@yukawallstudio](https://twitter.com/yukawallstudio) までお問い合わせください。内容は以下のテンプレートを参考にしてください。

```
RecotteStudio本体のVersion: (例) v1.4.0.0
RecottePluginのVersion: (例) v0.1.3
OS: (例) Windows 10 Pro
PCの型番: GARAGARA-3000-BT
エラーメッセージ: (出来ればスクリーンショットで添付)
その他:
クラッシュダンプ: (ある場合は "C:\Users\<ユーザー名>\AppData\Local\CrashDumps" 内のDMPファイルを添付する)
```

## ビルド環境

- VisualStudio2019 16.10.2

`std::format`とか使ってるので少し前のバージョンでも動かないと思います。

通常、`RecottePluginManager.dll`は`~\RecottePlugin`以下のPluginをロードしますが、環境変数`RECOTTE_PLUGIN_DIR`にディレクトリを設定しておくことで別ディレクトリからロードさせることもできます。

![](img/env.png)

## 新しいPluginの開発方法

このリポジトリをフォークして拡張するのではなく、別のプロジェクトとして「`~/RecottePlugin`以下にコピーしてインストールしてね」というスタイルを取っていただけると助かります。

`LayerFolding.cpp`あたりを参考にして、`OnPluginStart`と`OnPluginFinish`をエクスポートしたDLLを作ってください。

結局のところRecotteStudio内部の処理をもりもり書き換えないことには何もできないので覚悟が必要です。一応動的に書き換えるためのUtilityが`HookHelper`で実装されているので必要に応じて使ってください。

## メモ

作りとしてはWin32のWindowシステムベースだけど、GDI+で独自の描画をしている個所が多いのでUIいじる系は結構大変？

- https://qiita.com/up-hash/items/28375739208402721323
- https://qiita.com/up-hash/items/8ca41c4038c26a96674a
- https://gist.github.com/wallstudio/b78ce70e015058f7c33e391b0cfd7815
- https://silight.hatenablog.jp/entry/2016/08/23/212820
