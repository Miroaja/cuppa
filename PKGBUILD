pkgname="cuppa"
pkgver="0.9.0"
pkgdesc="A hot cuppa joe"
url=https://github.com/Miroaja/cuppa
arch=("x86_x64" "arm")
depends=("cmake" "ncurses")
licence=("GPL-3.0-or-later")
source=("$pkgname::git://github.com/linuxdeepin/dtkcore.git")
sha512sums=("SKIP")

build() {
  cd $pkgname
  cmake .
  make
}

package() {
  cd $pkgname
  make INSTALL_ROOT="$pkgdir" install
}
