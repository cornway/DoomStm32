#ifndef     DMA_CHAIN_H
#define     DMA_CHAIN_H


#include "stdint.h"
#include "abstract.h"
#include "iterable.h"

class DmaFactory;
class DmaChanel;


class DmaChunk : Link<DmaChunk> {
    private :
        uint32_t dest, src;
        uint32_t size, dest_allign, src_allign;
        DmaChunk (uint32_t channel, uint32_t src, uint32_t dest, uint32_t src_all, uint32_t dest_all, uint32_t size);
        void IT_Handler ();
        virtual void DmaStart (uint32_t channel, uint32_t src, uint32_t dest, uint32_t src_all, uint32_t dest_all, uint32_t size);
        virtual void DmaStop (uint32_t channel);
    
        friend class DmaFactory;
        friend class DmaChanel;
    public : 
        
};

enum DmaChannelEventsEnum {
    DMA_CHUNK_COMPLETE_EVENT,
    DMA_TR_COMPLETE_EVENT,
    DMA_ABORT_EVENT,
};

class DmaChannel : Link<DmaChannel> {
    private :
        uint32_t id;
        vector::Vector<DmaChunk> ChunksArray;
        abstract::EventBurner eventsBurner;
        DmaChannel (uint32_t id);
        ~DmaChannel();
        void IT_Handler ();
        friend class DmaFactory;
    public :
        void start ();
        void abort ();
        void addEventListener (abstract::EventListener l);
        void removeAllListeners ();
        bool isBusy ();
        
};


class DmaFactory {
    private :
        vector::Vector<DmaChannel> channelsArray;   
        
    public :
        DmaFactory ();
        DmaChannel *newChannel (uint32_t id);
        void IT_Handler ();
        
};

#endif      /*DMA_CHAIN_H*/


