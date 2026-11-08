#include <stdint.h>

extern "C" {

// Colapsar los buffers COMMON de Master no utilizados a 8 bytes con alineación estricta de 8 bytes
alignas(8) uint8_t lctrMstExtInitTbl[8] = {0};
alignas(8) uint8_t lctrMstPerScanTbl[8] = {0};
alignas(8) uint8_t lctrMstExtScanOneMCtx[8] = {0};
alignas(8) uint8_t lctrMstExtScanCodedCtx[8] = {0};
alignas(8) uint8_t lctrMstScan[8] = {0};
alignas(8) uint8_t lctrMstInit[8] = {0};
alignas(8) uint8_t lctrMstExtScan[8] = {0};

// Stubs para anular cualquier inicialización o ejecución de Master (evita que se escriba en los buffers colapsados)
void __wrap_LlScanMasterInit(void) {}
void __wrap_LlInitMasterInit(void) {}
void __wrap_LlEncConnMasterInit(void) {}
void __wrap_LlExtScanMasterInit(void) {}
void __wrap_LlExtInitMasterInit(void) {}
void __wrap_LlPhyMasterInit(void) {}

// Stubs para LHCI Master
void __wrap_LhciScanMasterInit(void) {}
void __wrap_LhciEncMasterInit(void) {}
void __wrap_LhciConnMasterInit(void) {}
void __wrap_LhciExtScanMasterInit(void) {}
void __wrap_LhciExtConnMasterInit(void) {}

} // extern "C"
