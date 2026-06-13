#!/usr/bin/env bash
set -E -e -u -o pipefail

if [[ ! -d /root/.oh-my-zsh ]]; then
	sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
fi
if [[ ! -d ~/.oh-my-zsh/custom/plugins/zsh-autosuggestions ]]; then
	git clone https://github.com/zsh-users/zsh-autosuggestions ~/.oh-my-zsh/custom/plugins/zsh-autosuggestions
fi
sed -i "s/plugins=(git)/plugins=(git zsh-autosuggestions)/g" ~/.zshrc
if [[ ! -d ~/.dotfiles ]]; then
	git clone https://github.com/Martins3/My-Linux-Config ~/.dotfiles
fi
bash ~/.dotfiles/scripts/install.sh
nvim --headless +qall

# go 和 cargo 都是安装到 home 的，所以到 guest 中执行
proxy_ip="10.0.2.2"
export https_proxy=http://$proxy_ip:8889
export http_proxy=http://$proxy_ip:8889
export HTTPS_PROXY=http://$proxy_ip:8889
export HTTP_PROXY=http://$proxy_ip:8889

go install github.com/sachaos/viddy@latest
go install github.com/charmbracelet/gum@latest

# cargo install atuin 不行，使用脚本更加简单
curl --proto '=https' --tlsv1.2 -LsSf https://setup.atuin.sh | sh
echo 'source ~/core/vn/code/zsh' >>~/.zshrc
echo 'source ~/.dotfiles/config/zsh' >>~/.zshrc

cargo install --locked pueue

echo "最后，ctrl+I"
echo "最后，setup_ccls"
