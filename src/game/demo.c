#include "demo.h"

struct DemoInputObj gDemoInputs = {
    7, NULL,
    {
        { offsetof(struct DemoInputObj, bitdw), sizeof(gDemoInputs.bitdw) },
        { offsetof(struct DemoInputObj, wf), sizeof(gDemoInputs.wf) },
        { offsetof(struct DemoInputObj, ccm), sizeof(gDemoInputs.ccm) },
        { offsetof(struct DemoInputObj, bbh), sizeof(gDemoInputs.bbh) },
        { offsetof(struct DemoInputObj, jrb), sizeof(gDemoInputs.jrb) },
        { offsetof(struct DemoInputObj, hmc), sizeof(gDemoInputs.hmc) },
        { offsetof(struct DemoInputObj, pss), sizeof(gDemoInputs.pss) },
    }
};
