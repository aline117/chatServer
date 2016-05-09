echo "build st-1.9t";
_ST_MAKE=linux-optimized && _ST_EXTRA_CFLAGS="-DMD_HAVE_EPOLL" 
#_ST_MAKE=linux-debug && _ST_EXTRA_CFLAGS="-DMD_HAVE_EPOLL" 
                #rm -rf ${SRS_OBJS}/st-1.9 && cd ${SRS_OBJS} && 
		[ ! -d st-1.9 ] &&
                unzip -q st-1.9.zip && cd st-1.9 && chmod +w * &&
                patch -p0 < ../patches/1.st.arm.patch &&
                patch -p0 < ../patches/3.st.osx.kqueue.patch &&
                patch -p0 < ../patches/4.st.disable.examples.patch #&&

                #make ${_ST_MAKE} EXTRA_CFLAGS="${_ST_EXTRA_CFLAGS}" #&&
#                cd .. && rm -rf st && ln -sf st-1.9/obj st &&
#                cd .. && rm -f ${SRS_OBJS}/_flag.st.cross.build.tmp

                make ${_ST_MAKE} EXTRA_CFLAGS="${_ST_EXTRA_CFLAGS}"
