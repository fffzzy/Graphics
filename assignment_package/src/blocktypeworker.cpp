#include "blocktypeworker.h"

BlockTypeWorker::BlockTypeWorker(int x, int y, vector<Chunk *> chunks,
                                 vector<Chunk*> *chunksThatHaveBlockData,
                                 QMutex *chunksThatHaveBlockDataLock)
    : PosX(x), PosY(y), chunks(chunks),
      chunksThatHaveBlockData(chunksThatHaveBlockData),
      chunksThatHaveBlockDataLock(chunksThatHaveBlockDataLock)
{
}

void BlockTypeWorker::run()
{
    for (auto &chunk : chunks)
    {
        chunk->generateChunk();
        chunksThatHaveBlockDataLock->lock();
        chunksThatHaveBlockData->push_back(chunk);
        chunksThatHaveBlockDataLock->unlock();
    }
}
