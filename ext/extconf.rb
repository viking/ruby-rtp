require 'mkmf'

$LIBS << " -lortp"
create_makefile('rtp')
