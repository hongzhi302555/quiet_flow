DoubleConversionDir = cpp3rdlib/double-conversion

libdoubleconversion: $(DoubleConversionDir)/lib/libdoubleconversion.a

double_conversion_temp:
	mkdir -p double_conversion_temp
	# cp double_conversion.zip double_conversion_temp/a.zip
	curl -sL https://github.com/google/double-conversion/archive/refs/tags/v3.2.1.zip > double_conversion_temp/a.zip
	unzip double_conversion_temp/a.zip -d double_conversion_temp/
	cp -r double_conversion_temp/double-conversion-3.2.1/double-conversion $(DoubleConversionDir)/include
	cd double_conversion_temp/double-conversion-3.2.1; make
	mv double_conversion_temp/double-conversion-3.2.1/libdouble-conversion.a $(DoubleConversionDir)/lib/libdoubleconversion.a
	rm -rf double_conversion_temp

$(DoubleConversionDir)/lib/libdoubleconversion.a:
	mkdir -p $(DoubleConversionDir)/lib $(DoubleConversionDir)/include
	if [ ! -f "$@" ]; then make double_conversion_temp; fi


EventDir = cpp3rdlib/libevent

libevent: $(EventDir)/lib/libevent.a

libevent_temp:
	mkdir -p libevent_temp
	# cp libevent.zip libevent_temp/a.zip
	curl -sL https://github.com/libevent/libevent/archive/refs/tags/release-2.1.12-stable.zip > libevent_temp/a.zip
	unzip libevent_temp/a.zip -d libevent_temp/
	cd libevent_temp/libevent-release-2.1.12-stable;mkdir build && cd build && cmake .. && make
	mv libevent_temp/libevent-release-2.1.12-stable/build/lib/libevent.a $(EventDir)/lib/libevent.a
	rm -rf libevent_temp

$(EventDir)/lib/libevent.a:
	mkdir -p $(EventDir)/lib
	if [ ! -f "$@" ]; then make libevent_temp; fi


GlogDir = cpp3rdlib/glog

libglog: $(GlogDir)/lib/libglog.so

glog_temp:
	mkdir -p glog_temp
	# cp libglog.zip glog_temp/a.zip
	curl -sL https://github.com/google/glog/archive/refs/tags/v0.3.5.zip > glog_temp/a.zip
	unzip glog_temp/a.zip -d glog_temp/
	cd glog_temp/glog-0.3.5;./configure && make
	mv glog_temp/glog-0.3.5/.libs/libglog.so.0.0.0 $(GlogDir)/lib/libglog.so
	mv glog_temp/glog-0.3.5/src/glog $(GlogDir)/include/glog
	rm -rf glog_temp

$(GlogDir)/lib/libglog.a:
	mkdir -p $(GlogDir)/lib  $(GlogDir)/include
	if [ ! -f "$@" ]; then make glog_temp; fi


# BoostContextDir = cpp3rdlib/boostcontext

# boostcontext: $(BoostContextDir)/lib/libboost_context.a

# boost_temp:
# 	mkdir -p boost_temp
# 	cp boost.zip boost_temp/a.zip
# 	# curl -sL https://github.com/boostorg/boost/archive/refs/tags/boost-1.56.0.zip > boost_temp/a.zip
# 	unzip boost_temp/a.zip -d boost_temp/
# 	cd boost_temp/boost-boost-1.56.0;./configure && make
# 	mv boost_temp/boost-boost-1.56.0/.libs/libglog.so.0.0.0 $(BoostContextDir)/lib/libglog.so
# 	mv boost_temp/boost-boost-1.56.0/src/glog $(BoostContextDir)/include/glog
# 	rm -rf boost_temp

# $(BoostContextDir)/lib/libboost_context.a:
# 	mkdir -p $(BoostContextDir)/lib  $(BoostContextDir)/include
# 	if [ ! -f "$@" ]; then make boost_temp; fi


# https://github.com/boostorg/boost/archive/refs/tags/boost-1.56.0.zip