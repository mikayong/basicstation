
TD=../..
include ${TD}/setup.gmk
PLT=platform-${platform}

LGWLIB=${TD}/${BD}/lib/liblgw.a
LGWINC=${TD}/${BD}/include/lgw
LGWPKT=${TD}/${BD}/bin/util_pkt_logger
LGWTXC=${TD}/${BD}/bin/util_tx_continuous
LGWTST=${TD}/${BD}/tst

HFILES = $(wildcard ${PLT}/libloragw/inc/*.h)
SRCS = $(wildcard ${PLT}/libloragw/inc/*.c)
ARTEFACTS = ${LGWLIB} ${LGWINC} ${LGWPKT} ${LGWTXC} ${LGWTST} $(patsubst %, ${LGWINC}/%, $(notdir ${HFILES}) config.h)

all: $(if ${HFILES},${ARTEFACTS},${PLT})

${LGWINC}:
	@mkdir -p $@

${LGWTST}: ${PLT}/libloragw/libloragw.a 
	@mkdir -p $@
	@echo "  INSTALL LGW TESTSUIT"
	@cp ${PLT}/libloragw/test_loragw_* $@

${LGWLIB}: ${PLT}/libloragw/libloragw.a
	@mkdir -p ${@D}
	@echo "  CP    ${<F} -> $@"
	@cp $< $@

${LGWPKT}: ${PLT}/util_pkt_logger/util_pkt_logger
	@mkdir -p ${@D}
	@echo "  CP    ${<F} -> $@"
	@cp $< $@

${LGWTXC}: ${PLT}/util_tx_continuous/util_tx_continuous
	@mkdir -p ${@D}
	@echo "  CP    ${<F} -> $@"
	@cp $< $@

${PLT}/libloragw/libloragw.a: ${SRCS} ${HFILES}
	${MAKE} -C ${PLT}/libloragw all CC=${CC} AR=${AR} LD=${LD} TDfull=${TDfull}

${PLT}/libloragw/inc/config.h: ${PLT}/libloragw/library.cfg
	${MAKE} -C ${PLT}/libloragw inc/config.h CC=${CC} AR=${AR} LD=${LD} TDfull=${TDfull}

${LGWINC}/%.h: ${PLT}/libloragw/inc/%.h | ${LGWINC}
	@echo "  CP    ${<F} -> $@"
	@cp $< $@

${PLT}: prep.sh
	platform=${platform} variant=${variant} ./prep.sh
	$(MAKE) --no-print-directory all platform=${platform} variant=${variant}

${PLT}/util_pkt_logger/util_pkt_logger: ${PLT}/libloragw/libloragw.a
	${MAKE} -C ${PLT}/util_pkt_logger CC=${CC} AR=${AR} LD=${LD} TDfull=${TDfull} 

${PLT}/util_tx_continuous/util_tx_continuous: ${PLT}/libloragw/libloragw.a
	${MAKE} -C ${PLT}/util_tx_continuous CC=${CC} AR=${AR} LD=${LD} TDfull=${TDfull} 

clean:
	rm -rf ${ARTEFACTS}
	if [ -d ${PLT} ]; then ${MAKE} -C ${PLT}/libloragw clean; fi

super-clean: clean
	rm -rf git-repo platform-*

.PHONY: all clean super-clean platform-${platform}
