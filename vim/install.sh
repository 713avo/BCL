#!/bin/bash
################################################################################
# BCL Vim Syntax Installation Script
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BCL_VIM="${SCRIPT_DIR}/bcl.vim"
FTDETECT="${SCRIPT_DIR}/ftdetect.vim"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "═══════════════════════════════════════════════════════════════"
echo "  BCL Vim Syntax Highlighting - Installation Script"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Check if files exist
if [ ! -f "$BCL_VIM" ]; then
    echo -e "${RED}Error: bcl.vim not found!${NC}"
    exit 1
fi

if [ ! -f "$FTDETECT" ]; then
    echo -e "${RED}Error: ftdetect.vim not found!${NC}"
    exit 1
fi

# Ask installation type
echo "Choose installation method:"
echo "  1) User installation (recommended) - ~/.vim/"
echo "  2) System-wide installation - /usr/share/vim/"
echo "  3) Vim 8+ pack installation - ~/.vim/pack/"
echo ""
read -p "Enter choice [1-3]: " choice

case $choice in
    1)
        # User installation
        echo -e "${YELLOW}Installing for current user...${NC}"
        mkdir -p ~/.vim/syntax
        mkdir -p ~/.vim/ftdetect
        cp "$BCL_VIM" ~/.vim/syntax/
        cp "$FTDETECT" ~/.vim/ftdetect/bcl.vim
        echo -e "${GREEN}✓ Installed to ~/.vim/${NC}"
        echo ""
        echo "BCL syntax highlighting is now enabled!"
        echo "Open any .bcl file in Vim to test it."
        ;;
    2)
        # System-wide installation
        echo -e "${YELLOW}Installing system-wide (requires sudo)...${NC}"
        sudo mkdir -p /usr/share/vim/vimfiles/syntax
        sudo mkdir -p /usr/share/vim/vimfiles/ftdetect
        sudo cp "$BCL_VIM" /usr/share/vim/vimfiles/syntax/
        sudo cp "$FTDETECT" /usr/share/vim/vimfiles/ftdetect/bcl.vim
        echo -e "${GREEN}✓ Installed to /usr/share/vim/vimfiles/${NC}"
        echo ""
        echo "BCL syntax highlighting is now enabled system-wide!"
        ;;
    3)
        # Pack installation
        echo -e "${YELLOW}Installing as Vim package...${NC}"
        mkdir -p ~/.vim/pack/bcl/start/bcl-syntax/syntax
        mkdir -p ~/.vim/pack/bcl/start/bcl-syntax/ftdetect
        cp "$BCL_VIM" ~/.vim/pack/bcl/start/bcl-syntax/syntax/
        cp "$FTDETECT" ~/.vim/pack/bcl/start/bcl-syntax/ftdetect/bcl.vim
        echo -e "${GREEN}✓ Installed to ~/.vim/pack/bcl/start/bcl-syntax/${NC}"
        echo ""
        echo "BCL syntax highlighting is now enabled!"
        ;;
    *)
        echo -e "${RED}Invalid choice!${NC}"
        exit 1
        ;;
esac

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Installation complete!"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Test it with:"
echo "  vim examples/foreach_demo.bcl"
echo ""
echo "For more information, see vim/README.md"
