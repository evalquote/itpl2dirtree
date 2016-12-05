itpl2dirtree                         2016.12.5

Mac iTunes の音楽プレイリストファイル (~/Music/iTunes/iTunes Music Library.xml) に
したがって Linux (や Mac) にディレクトリ木を生成します. iTunes の音楽ファイルフォルダ
(~/Music/iTunes/iTunes Media/Music) をコピーし, コピー先ディレクトリを指定すれば, 
生成したディレクトリ木に音楽のファイルへのシンボリックリンクを生成します. 

itpl2dirtreeは, iTunes ライブラリを iTunes でない環境にコピーして音楽を
聴くときに便利です. 例えば, Rasberry pi の Volumio です. 

expat-dev を使用しています. システムに expat-dev がなければ
インストールしてください. volumio なら次のように実行します. 

apt-get install expat-dev

使いかた
itpl2dirtree を make して適切な場所に置いて下さい. 


itpl2dirtree -p music data ath -i original music data path [-o output] [-n] < file.xml

-p: 音楽ファイルをコピーしたディレクトリパスを指定します
-i: playlistの音楽ファイル名から取り除く文字列を指定します. 
-o: 生成するディレクトリのトップのパスをしていします. デフォルトは ./playlist です. 
-n: XML ファイルを読み込んでチェックしますが, 実際のディレクトリは作りません. 
標準入力  iTunes library XML ファイルの内容を読み込ませます. 

iTunes library XML ファイルは Mac の次のファイルです. 
~/Music/iTunes/iTunes Music Library.

もし Mac にこのファイルがなければ, 
https://support.apple.com/ja-jp/HT201610
を参考にしてください. 

-i オプションについて
iTunes library XML ファイルには Mac での音楽ファイルの絶対パスが記述されているので, 
そのパスの前方を削除して使用します. これが -i オプションです. 

-i オプションに指定する文字列を探すには XML ファイルから Location が書かれている行を
探します. 次のように実行します. 

grep Location ~/Music/iTunes/'iTunes Music Library.xml' | head -1

実行結果が例えば次のようなら

<key>Location</key><string>file:///Users/evalquote/Music/iTunes/iTunes%20Media/Music/Kenneth%20Gilbert/Bach_%20Italian%20Concerto,%204%20Duos,%20Etc_/09%20Bach_%20Overture%20In%20The%20French%20Style,%20BWV%20831%20-%202.%20Courante.m4a</string>

-i file:///Users/evalquote/Music/iTunes/iTunes%20Media/

のように指定します. 

実行例: (Mac から Volumio)

Mac上で実行:

rsync -av --delete  ~/Music/iTunes/'iTunes Media/Music' volumio:Music/data
scp -p ~/Music/iTunes/'iTunes Music Library.xml' volumio:Music/itpl.xml

次に Volumio 上で実行
cd /mnt/USB
cat ~/Music/itpl.xml | itpl2dirtree -p ~/Music/data -i 'file:///Users/evalquote/Music/iTunes/iTunes%20Media/'

以上
