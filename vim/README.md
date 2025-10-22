# BCL Syntax Highlighting for Vim

This directory contains Vim syntax highlighting for BCL (Basic Command Language) files.

## Features

- **Control Structures**: IF, WHILE, FOR, FOREACH, SWITCH, etc.
- **Procedures**: PROC, RETURN, GLOBAL
- **Variables**: Variable references with `$` prefix, array syntax `$array(index)`
- **Commands**: All BCL built-in commands (LIST, STRING, FILE, REGEXP, etc.)
- **String Literals**: Double-quoted strings with escape sequences
- **Comments**: Lines starting with `#`
- **Command Substitution**: `[...]` syntax
- **Numbers**: Integers, floats, and hexadecimal
- **Operators**: Arithmetic, comparison, and logical operators

## Installation

### Method 1: User Installation (Recommended)

Copy the syntax file to your Vim syntax directory:

```bash
mkdir -p ~/.vim/syntax
cp vim/bcl.vim ~/.vim/syntax/
```

Create or edit `~/.vim/ftdetect/bcl.vim`:

```bash
mkdir -p ~/.vim/ftdetect
echo 'au BufRead,BufNewFile *.bcl set filetype=bcl' > ~/.vim/ftdetect/bcl.vim
```

### Method 2: System-wide Installation

For system-wide installation (requires root):

```bash
sudo cp vim/bcl.vim /usr/share/vim/vimfiles/syntax/
sudo mkdir -p /usr/share/vim/vimfiles/ftdetect
echo 'au BufRead,BufNewFile *.bcl set filetype=bcl' | sudo tee /usr/share/vim/vimfiles/ftdetect/bcl.vim
```

### Method 3: Using Vim's packpath (Vim 8+)

```bash
mkdir -p ~/.vim/pack/bcl/start/bcl-syntax/{syntax,ftdetect}
cp vim/bcl.vim ~/.vim/pack/bcl/start/bcl-syntax/syntax/
echo 'au BufRead,BufNewFile *.bcl set filetype=bcl' > ~/.vim/pack/bcl/start/bcl-syntax/ftdetect/bcl.vim
```

## Usage

Once installed, Vim will automatically detect `.bcl` files and apply syntax highlighting.

You can also manually set the filetype for any file:

```vim
:set filetype=bcl
```

Or add this to the top of your BCL scripts:

```bcl
# vim: set filetype=bcl:
```

## Testing

Open any BCL file to test the syntax highlighting:

```bash
vim examples/foreach_demo.bcl
```

## Color Scheme

The syntax file uses standard Vim highlight groups, so it will work with any color scheme. The default mappings are:

- **Keywords**: Statement (control structures, loops)
- **Functions**: Function (built-in commands)
- **Strings**: String
- **Comments**: Comment
- **Numbers**: Number
- **Operators**: Operator
- **Variables**: Identifier
- **Special**: Command substitution `[...]`

## Customization

You can customize colors by adding to your `~/.vimrc`:

```vim
" Example: Make BCL keywords bold and blue
hi bclControl ctermfg=blue cterm=bold guifg=blue gui=bold

" Example: Make BCL variables green
hi bclVar ctermfg=green guifg=green
```

## Examples

The syntax file recognizes:

```bcl
#!/usr/bin/env bcl

# Comments are highlighted
SET message "Hello, World!"  # Variables and strings

# Control structures
IF [EXPR $count > 10] THEN
    PUTS "Count is high"
ELSEIF [EXPR $count == 5] THEN
    PUTS "Count is five"
ELSE
    PUTS "Count is low"
END

# Loops
FOREACH item IN $list DO
    PUTS "Item: $item"
END

# Procedures
PROC calculate WITH x y DO
    SET result [EXPR $x + $y]
    RETURN $result
END

# Arrays
SET array(key) "value"
PUTS $array(key)

# Command substitution
SET files [GLOB "/tmp/*.txt"]
```

## License

This syntax file is part of the BCL project and follows the same license.

## Contributing

To improve the syntax highlighting, edit `vim/bcl.vim` and submit a pull request.
