#ifndef BLOCKTYPEWORKER_H
#define BLOCKTYPEWORKER_H

#include <QRunnable>
#include <QMutex>
#include "scene/terrain.h"
using namespace std;

class BlockTypeWorker : public QRunnable
{
protected:
    int PosX, PosY;
    vector<Chunk *> chunks;
    vector<Chunk *> *chunksThatHaveBlockData;
    QMutex *chunksThatHaveBlockDataLock;

public:
    BlockTypeWorker(int x, int y, vector<Chunk *> chunks,
                    vector<Chunk *> *chunksThatHaveBlockData,
                    QMutex *chunksThatHaveBlockDataLock);
    void run() override;
};

#endif // BLOCKTYPEWORKER_H
