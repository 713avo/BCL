" Vim filetype detection file
" Language: BCL (Basic Command Language)

" Detect .bcl files
au BufRead,BufNewFile *.bcl set filetype=bcl

" Detect .BLB files (BCL libraries)
au BufRead,BufNewFile *.BLB set filetype=bcl
au BufRead,BufNewFile *.blb set filetype=bcl

" Detect BCL shebang
au BufRead,BufNewFile * if getline(1) =~ '^#!.*\<bcl\>' | setfiletype bcl | endif
