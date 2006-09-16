# Copyright 2006 Brice Arnould
# Distributed under the terms of the GNU General Public License
# version 2, or (at your option) any later version

DESCRIPTION="Defragmenter (rewrite fragmented or misplaced files)"
SRC_URI="http://download.savannah.nongnu.org/releases/shake/shake-${PV}.tar.bz2"
HOMEPAGE="http://vleu.net/shake/ http://savannah.nongnu.org/projects/shake"

LICENSE="GPL-2 FDL-1.2"
SLOT="0"
KEYWORDS="~x86 ~amd64 ~ppc"
IUSE="static"
DEPEND="sys-apps/attr"

src_compile() {
	use static && make static
	make all
}

src_install() {
	use static && dosbin shake-static
	dosbin shake
	dobin unattr
	doman doc/shake.8 doc/unattr.8
}
