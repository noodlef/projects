
"显示行号 
set nu 

"启动时隐去援助提示 
set shortmess=atI 

"语法高亮 
syntax on

"使用vim的键盘模式 
set nocompatible 

"不需要备份 
set nobackup

"没有保存或文件只读时弹出确认 
set confirm 

"鼠标可用 
set mouse=a 

"tab缩进 
set tabstop=4 
set shiftwidth=4 
set expandtab 
set smarttab 

"文件自动检测外部更改
set autoread 

"c文件自动缩进 
set cindent 

"自动对齐 
set autoindent 

"智能缩进
set smartindent 

"高亮查找匹配 
set hlsearch 

"背景色 
set background=dark

"显示匹配 
set showmatch 

"显示标尺，就是在右下角显示光标位置 
set ruler

"去除vi的一致性 
set nocompatible 

"允许折叠 
set foldenable
""""""""""""""""""设置折叠"""""""""""""""""""""  
"

"根据语法折叠 
set fdm=syntax

"手动折叠 
set fdm=manual 

"设置键盘映射，通过空格设置折叠 
nnoremap <space> @=((foldclosed(line('.')<0)?'zc':'zo'))<CR>
""""""""""""""""""""""""""""""""""""""""""""""" 

"不要闪烁 
set novisualbell

"启动显示状态行 
set laststatus=2 

"浅色显示当前行 
autocmd InsertLeave * se nocul 

"用浅色高亮当前行 
autocmd InsertEnter * se cul 

"显示输入的命令 
set showcmd 

"被分割窗口之间显示空白 
set fillchars=vert:/ 
set fillchars=stl:/ 
set fillchars=stlnc:/

"编码设置
set encoding=utf-8
set fileencodings=cp936,gbk,gb2312,utf-8,ucs-bom,latin1,big5
set fileencoding=utf-8
let &termencoding=&encoding
"language messages zh_CN.utf-8

"括号自动补全
inoremap ( ()<ESC>i
inoremap [ []<ESC>i
inoremap " ""<ESC>i
inoremap ' ''<ESC>i
imap { {}<ESC>i<CR><ESC>V<O
