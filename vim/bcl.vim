" Vim syntax file
" Language: BCL (Basic Command Language)
" Maintainer: BCL Development Team
" Latest Revision: 2025

if exists("b:current_syntax")
  finish
endif

" Keywords - Control structures
syn keyword bclControl IF THEN ELSEIF ELSE END WHILE DO FOR TO STEP FOREACH IN
syn keyword bclControl SWITCH CASE DEFAULT BREAK CONTINUE EXIT RETURN

" Keywords - Procedures
syn keyword bclProc PROC WITH GLOBAL

" Keywords - Variables
syn keyword bclVariable SET UNSET INCR APPEND

" Keywords - I/O
syn keyword bclIO PUTS PUTSN GETS OPEN CLOSE READ

" Keywords - Lists
syn keyword bclList LIST SPLIT JOIN LINDEX LRANGE LLENGTH LAPPEND LINSERT
syn keyword bclList LREPLACE LSORT LSEARCH CONCAT

" Keywords - Strings
syn keyword bclString STRING FORMAT SCAN

" Keywords - Files
syn keyword bclFile FILE EXISTS SIZE DELETE RENAME PWD GLOB DIRECTORY TAILS
syn keyword bclFile NOCOMPLAIN TYPES EOF TELL SEEK

" Keywords - Regular expressions
syn keyword bclRegexp REGEXP REGSUB NOCASE LINE LINESTOP LINEANCHOR EXPANDED
syn keyword bclRegexp ALL INDICES START MATCH SUBMATCHES COUNT

" Keywords - Time
syn keyword bclClock CLOCK SECONDS MILLISECONDS MICROSECONDS TIMEZONE LOCALE GMT BASE

" Keywords - System
syn keyword bclSystem EVAL SOURCE AFTER EXEC ENV ARGV

" Keywords - Introspection
syn keyword bclInfo INFO COMMANDS GLOBALS LOCALS PROCS VARS ARGS BCLVERSION

" Keywords - Expressions
syn keyword bclExpr EXPR AND OR NOT

" Keywords - Arrays
syn keyword bclArray ARRAY GET UNSET EXISTS SIZE NAMES KEYS VALUES

" Keywords - Binary
syn keyword bclBinary BINARY FORMAT SCAN

" String literals
syn region bclString start='"' end='"' contains=bclVariable,bclEscape
syn match bclEscape '\\[nrt\\"]' contained

" Variables
syn match bclVar '\$[a-zA-Z_][a-zA-Z0-9_]*'
syn match bclArrayVar '\$[a-zA-Z_][a-zA-Z0-9_]*([^)]*)'

" Numbers
syn match bclNumber '\<\d\+\>'
syn match bclNumber '\<\d\+\.\d\+\>'
syn match bclNumber '\<0x[0-9a-fA-F]\+\>'

" Comments
syn match bclComment '#.*$'

" Command substitution
syn region bclCommand start='\[' end='\]' contains=ALL

" Operators
syn match bclOperator '=='
syn match bclOperator '!='
syn match bclOperator '<='
syn match bclOperator '>='
syn match bclOperator '<'
syn match bclOperator '>'
syn match bclOperator '+'
syn match bclOperator '-'
syn match bclOperator '\*'
syn match bclOperator '/'
syn match bclOperator '%'
syn match bclOperator '&&'
syn match bclOperator '||'
syn match bclOperator '!'

" Shebang
syn match bclShebang '\%^#!.*bcl'

" Link to standard highlight groups
hi def link bclControl Statement
hi def link bclProc Function
hi def link bclVariable Identifier
hi def link bclIO Function
hi def link bclList Function
hi def link bclString String
hi def link bclFile Function
hi def link bclRegexp Function
hi def link bclClock Function
hi def link bclSystem Function
hi def link bclInfo Function
hi def link bclExpr Operator
hi def link bclArray Function
hi def link bclBinary Function
hi def link bclVar Identifier
hi def link bclArrayVar Identifier
hi def link bclNumber Number
hi def link bclComment Comment
hi def link bclCommand Special
hi def link bclOperator Operator
hi def link bclEscape SpecialChar
hi def link bclShebang PreProc

let b:current_syntax = "bcl"
