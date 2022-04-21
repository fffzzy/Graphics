#include "vboworker.h"
#include <iostream>

VBOWorker::VBOWorker(Chunk* c, vector<ChunkVBOData>* v, QMutex* m): chunk(c), chunksThatHaveVBOs(v), chunksThatHaveVBOsLock(m)
{}

void VBOWorker::run() {
    // try {
    chunk->createVBOdata();
    // }
    // catch(std::exception &e) {
    //     std::cout << "vbo worker crashed" << std::endl;
    // }

    chunksThatHaveVBOsLock->lock();
    chunksThatHaveVBOs->push_back(chunk->m_chunkVBOData);
    chunksThatHaveVBOsLock->unlock();
}
