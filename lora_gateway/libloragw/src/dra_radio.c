/**
 * Author: Dragino 
 * Date: 16/01/2018
 * 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.dragino.com
 *
 */

#include "radio.h"

static const uint8_t rxlorairqmask[] = {
    [RXMODE_SINGLE] = IRQ_LORA_RXDONE_MASK|IRQ_LORA_RXTOUT_MASK|IRQ_LORA_CRCERR_MASK,
    [RXMODE_SCAN]   = IRQ_LORA_RXDONE_MASK|IRQ_LORA_CRCERR_MASK,
    [RXMODE_RSSI]   = 0x00,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t readReg(void *spi_target, uint8_t addr)
{
    uint8_t data = 0x00;

    dra_sx127x_reg_r(spi_target, addr, &data);

    return data;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int writeReg(void *spi_target, uint8_t addr, uint8_t value)
{
    return dra_sx127x_reg_w(spi_target, addr, value);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void opmode (void *spi_target, uint8_t mode) {
    writeReg(spi_target, SX1276_REG_LR_OPMODE, (readReg(spi_target, SX1276_REG_LR_OPMODE) & ~OPMODE_MASK) | mode);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void opmodeLora(void *spi_target) {
    uint8_t u = OPMODE_LORA | 0x8; // TBD: sx1276 high freq
    writeReg(spi_target, SX1276_REG_LR_OPMODE, u);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setpower(void *spi_target, uint8_t pw) {
    writeReg(spi_target, REG_PADAC, 0x87);
    if (pw < 5) pw = 5;
    if (pw > 20) pw = 20;
    writeReg(spi_target, REG_PACONFIG, (uint16_t)(0x80 | ((pw - 5) & 0x0f)));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setfreq(void *spi_target, long frequency)
{
    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

    writeReg(spi_target, REG_FRF_MSB, (uint8_t)(frf >> 16));
    writeReg(spi_target, REG_FRF_MID, (uint8_t)(frf >> 8));
    writeReg(spi_target, REG_FRF_LSB, (uint8_t)(frf >> 0));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setsf(void *spi_target, int sf)
{
    if (sf < 6) {
        sf = 6;
    } else if (sf > 12) {
        sf = 12;
    }

    if (sf == 6) {
        writeReg(spi_target, REG_DETECTION_OPTIMIZE, 0xc5);
        writeReg(spi_target, REG_DETECTION_THRESHOLD, 0x0c);
    } else {
        writeReg(spi_target, REG_DETECTION_OPTIMIZE, 0xc3);
        writeReg(spi_target, REG_DETECTION_THRESHOLD, 0x0a);
    }

    writeReg(spi_target, REG_MODEM_CONFIG2, (readReg(spi_target, REG_MODEM_CONFIG2) & 0x0f) | ((sf << 4) & 0xf0));
    //MSG_LOG(DEBUG_SPI, "SPI", "SF=0x%02x\n", sf, readReg(spi_target, REG_MODEM_CONFIG2));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setsbw(void *spi_target, long sbw)
{
    int bw;

    if (sbw <= 7.8E3) {
        bw = 0;
    } else if (sbw <= 10.4E3) {
        bw = 1;
    } else if (sbw <= 15.6E3) {
        bw = 2;
    } else if (sbw <= 20.8E3) {
        bw = 3;
    } else if (sbw <= 31.25E3) {
        bw = 4;
    } else if (sbw <= 41.7E3) {
        bw = 5;
    } else if (sbw <= 62.5E3) {
        bw = 6;
    } else if (sbw <= 125E3) {
        bw = 7;
    } else if (sbw <= 250E3) {
        bw = 8;
    } else /*if (sbw <= 250E3)*/ {
        bw = 9;
    }

    writeReg(spi_target, REG_MODEM_CONFIG, (readReg(spi_target, REG_MODEM_CONFIG) & 0x0f) | (bw << 4));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setcr(void *spi_target, int denominator)
{
    if (denominator < 5) {
        denominator = 5;
    } else if (denominator > 8) {
        denominator = 8;
    }

    int cr = denominator - 4;

    writeReg(spi_target, REG_MODEM_CONFIG, (readReg(spi_target, REG_MODEM_CONFIG) & 0xf1) | (cr << 1));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setprlen(void *spi_target, long length)
{
    writeReg(spi_target, REG_PREAMBLE_MSB, (uint8_t)(length >> 8));
    writeReg(spi_target, REG_PREAMBLE_LSB, (uint8_t)(length >> 0));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void setsyncword(void *spi_target, int sw)
{
    writeReg(spi_target, REG_SYNC_WORD, sw);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void crccheck(void *spi_target, uint8_t nocrc)
{
    if (nocrc)
        writeReg(spi_target, REG_MODEM_CONFIG2, readReg(spi_target, REG_MODEM_CONFIG2) & 0xfb);
    else
        writeReg(spi_target, REG_MODEM_CONFIG2, readReg(spi_target, REG_MODEM_CONFIG2) | 0x04);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
// Lora configure : Freq, SF, BW
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool get_radio_version(void *spi_target, uint8_t rstpin)
{
    digitalWrite(rstpin, LOW);
    sleep(1);
    digitalWrite(rstpin, HIGH);
    sleep(1);

    opmode(spi_target, OPMODE_SLEEP);
    uint8_t version = readReg(spi_target, REG_VERSION);

    if (version == 0x12) {
        fprintf(stderr, "INFO~ spi_target(%d): SX1276 detected, starting.\n", (int)*spi_target);
        return true;
    } else {
        fprintf(stderr, "ERROR~ spi_target(%d): Unrecognized transceiver.\n", (int)*spi_target);
        return false;
    }

}

void setup_channel(radiodev *dev)
{
    MSG_LOG(DEBUG_INFO, "INFO~ Setup %s Channel: freq = %ld, sf = %d, syncwd=0x%02x, prlen=%d, cr=%d/5, spi=%d\n", \
            dev->desc, dev->freq, dev->sf, dev->syncword, dev->prlen, dev->cr - 1, dev->spiport);
    opmode(dev->spiport, OPMODE_SLEEP);
    opmodeLora(dev->spiport);
    ASSERT((readReg(dev->spiport, SX1276_REG_LR_OPMODE) & OPMODE_LORA) != 0);

    /* setup lora */
    setfreq(dev->spiport, dev->freq);
    setsf(dev->spiport, dev->sf);
    setsbw(dev->spiport, dev->bw);
    setcr(dev->spiport, dev->cr);
    setprlen(dev->spiport, dev->prlen);
    setsyncword(dev->spiport, dev->syncword);

    /* CRC check */
    crccheck(dev->spiport, dev->nocrc);

    // Boost on , 150% LNA current
    writeReg(dev->spiport, REG_LNA, LNA_MAX_GAIN);

    // Auto AGC Low datarate optiomize
    if (dev->sf < 11)
        writeReg(dev->spiport, REG_MODEM_CONFIG3, SX1276_MC3_AGCAUTO);
    else
        writeReg(dev->spiport, REG_MODEM_CONFIG3, SX1276_MC3_LOW_DATA_RATE_OPTIMIZE);

    //500kHz RX optimization
    writeReg(dev->spiport, 0x36, 0x02);
    writeReg(dev->spiport, 0x3a, 0x64);

    // configure output power,RFO pin Output power is limited to +14db
    //writeReg(dev->spiport, REG_PACONFIG, 0x8F);
    //writeReg(dev->spiport, REG_PADAC, readReg(dev->spiport, REG_PADAC)|0x07);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void rxlora(radiodev *dev, uint8_t rxmode)
{

    ASSERT((readReg(dev->spiport, SX1276_REG_LR_OPMODE) & OPMODE_LORA) != 0);

    /* enter standby mode (required for FIFO loading)) */
    opmode(dev->spiport, OPMODE_STANDBY);

    /* use inverted I/Q signal (prevent mote-to-mote communication) */
    
    if (dev->invertio) {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_ON | INVERTIQ_TX_OFF);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_ON);
    } else {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_OFF | INVERTIQ_TX_OFF);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_OFF);
    }


    if (rxmode == RXMODE_RSSI) { // use fixed settings for rssi scan
        writeReg(dev->spiport, REG_MODEM_CONFIG, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
        writeReg(dev->spiport, REG_MODEM_CONFIG2, RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
    }

    writeReg(dev->spiport, REG_MAX_PAYLOAD_LENGTH, 0x80);
    //writeReg(dev->spiport, REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
    writeReg(dev->spiport, REG_HOP_PERIOD, 0xFF);
    writeReg(dev->spiport, REG_FIFO_RX_BASE_AD, 0x00);
    writeReg(dev->spiport, REG_FIFO_ADDR_PTR, 0x00);

    // configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
    writeReg(dev->spiport, REG_DIO_MAPPING_1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_RXTOUT|MAP_DIO2_LORA_NOP);

    // clear all radio IRQ flags
    writeReg(dev->spiport, REG_IRQ_FLAGS, 0xFF);
    // enable required radio IRQs
    writeReg(dev->spiport, REG_IRQ_FLAGS_MASK, ~rxlorairqmask[rxmode]);

    //setsyncword(dev->spiport, LORA_MAC_PREAMBLE);  //LoraWan syncword

    // now instruct the radio to receive
    if (rxmode == RXMODE_SINGLE) { // single rx
        //printf("start rx_single\n");
        opmode(dev->spiport, OPMODE_RX_SINGLE);
        //writeReg(dev->spiport, SX1276_REG_LR_OPMODE, OPMODE_RX_SINGLE);
    } else { // continous rx (scan or rssi)
        opmode(dev->spiport, OPMODE_RX);
    }

}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool received(void *spi_target, struct pkt_rx_s *pkt_rx) {

    int i, rssicorr;

    int irqflags = readReg(spi_target, REG_IRQ_FLAGS);

    // clean all IRQ
    writeReg(spi_target, REG_IRQ_FLAGS, 0xFF);

    //printf("Start receive, flags=%d\n", irqflags);

    if ((irqflags & IRQ_LORA_RXDONE_MASK) && (irqflags & IRQ_LORA_CRCERR_MASK) == 0) {

        uint8_t currentAddr = readReg(spi_target, REG_FIFO_RX_CURRENT_ADDR);
        uint8_t receivedCount = readReg(spi_target, REG_RX_NB_BYTES);

        pkt_rx->size = receivedCount;

        writeReg(spi_target, REG_FIFO_ADDR_PTR, currentAddr);

        printf("\nRXTX~ Receive(HEX):");
        for(i = 0; i < receivedCount; i++) {
            pkt_rx->payload[i] = (char)readReg(spi_target, REG_FIFO);
            printf("%02x", pkt_rx->payload[i]);
        }
        printf("\n");

        uint8_t value = readReg(spi_target, REG_PKT_SNR_VALUE);

        if( value & 0x80 ) // The SNR sign bit is 1
        {
            // Invert and divide by 4
            value = ( ( ~value + 1 ) & 0xFF ) >> 2;
            pkt_rx->snr = -value;
        } else {
            // Divide by 4
            pkt_rx->snr = ( value & 0xFF ) >> 2;
        }
        
        rssicorr = 157;

        pkt_rx->rssi = readReg(spi_target, REG_PKTRSSI) - rssicorr;

        pkt_rx->empty = 0;  /* make sure save the received messaeg */

        return true;
    } /* else if (readReg(spi_target, SX1276_REG_LR_OPMODE) != (OPMODE_LORA | OPMODE_RX_SINGLE)) {  //single mode
        writeReg(spi_target, REG_FIFO_ADDR_PTR, 0x00);
        rxlora(spi_target, RXMODE_SINGLE);
    }*/
    return false;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void txlora(radiodev *dev, struct pkt_tx_s *pkt) {

    int i;

    struct timeval current_unix_time;
    uint32_t time_us;

    opmode(dev->spiport, OPMODE_SLEEP);
    // select LoRa modem (from sleep mode)
    opmodeLora(dev->spiport);

    ASSERT((readReg(dev->spiport, SX1276_REG_LR_OPMODE) & OPMODE_LORA) != 0);

    setpower(dev->spiport, pkt->rf_power);
    setfreq(dev->spiport, pkt->freq_hz);
    setsf(dev->spiport, sf_getval(pkt->datarate));
    setsbw(dev->spiport, bw_getval(pkt->bandwidth));
    setcr(dev->spiport, pkt->coderate);
    setprlen(dev->spiport, pkt->preamble);
    setsyncword(dev->spiport, dev->syncword);

    /* CRC check */
    crccheck(dev->spiport, pkt->no_crc);

    // Boost on , 150% LNA current
    writeReg(dev->spiport, REG_LNA, LNA_MAX_GAIN);

    writeReg(dev->spiport, REG_PARAMP, 0x08);

    // Auto AGC
    if (dev->sf < 11)
        writeReg(dev->spiport, REG_MODEM_CONFIG3, SX1276_MC3_AGCAUTO);
    else
        writeReg(dev->spiport, REG_MODEM_CONFIG3, SX1276_MC3_LOW_DATA_RATE_OPTIMIZE);

    // configure output power,RFO pin Output power is limited to +14db
    writeReg(dev->spiport, REG_PACONFIG, (uint8_t)(0x80|(15&0xf)));

    if (pkt->invert_pol) {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_OFF | INVERTIQ_TX_ON);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_ON);
    } else {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_OFF | INVERTIQ_TX_OFF);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_OFF);
    }

    // enter standby mode (required for FIFO loading))
    opmode(dev->spiport, OPMODE_STANDBY);

    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(dev->spiport, REG_DIO_MAPPING_1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(dev->spiport, REG_IRQ_FLAGS, 0xFF);
    // mask all IRQs but TxDone
    writeReg(dev->spiport, REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    // initialize the payload size and address pointers
    writeReg(dev->spiport, REG_FIFO_TX_BASE_AD, 0x00); writeReg(dev->spiport, REG_FIFO_ADDR_PTR, 0x00);

    for (i = 0; i < pkt->size; i++) { 
        writeReg(dev->spiport, REG_FIFO, pkt->payload[i]);
    }

    writeReg(dev->spiport, REG_PAYLOAD_LENGTH, pkt->size);

    gettimeofday(&current_unix_time, NULL);
    time_us = current_unix_time.tv_sec * 1000000UL + current_unix_time.tv_usec;

    time_us = pkt->count_us - 1495/*TX_START_DELAY*/ - time_us;

    if (pkt->tx_mode != IMMEDIATE && time_us > 0 && time_us < 30000/*TX_JIT DELAY*/)
        wait_us(time_us);

    // now we actually start the transmission
    opmode(dev->spiport, OPMODE_TX);

    // wait for TX done
    while(digitalRead(dev->dio[0]) == 0);

    MSG_LOG(DEBUG_INFO, "\nINFO~ Transmit at SF%iBW%ld on %.6lf.\n", sf_getval(pkt->datarate), bw_getval(pkt->bandwidth)/1000, (double)(pkt->freq_hz)/1000000);

    // mask all IRQs
    writeReg(dev->spiport, REG_IRQ_FLAGS_MASK, 0xFF);

    // clear all radio IRQ flags
    writeReg(dev->spiport, REG_IRQ_FLAGS, 0xFF);

    // go from stanby to sleep
    opmode(dev->spiport, OPMODE_SLEEP);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void single_tx(radiodev *dev, uint8_t *payload, int size) {

    ASSERT((readReg(dev->spiport, SX1276_REG_LR_OPMODE) & OPMODE_LORA) != 0);

    // enter standby mode (required for FIFO loading))
    opmode(dev->spiport, OPMODE_STANDBY);

    if (dev->invertio) {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_OFF | INVERTIQ_TX_ON);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_ON);
    } else {
        writeReg(dev->spiport, REG_INVERTIQ, (readReg(dev->spiport, REG_INVERTIQ) & INVERTIQ_RX_MASK & INVERTIQ_TX_MASK) | INVERTIQ_RX_OFF | INVERTIQ_TX_OFF);
        writeReg(dev->spiport, REG_INVERTIQ2, INVERTIQ2_OFF);
    }

    setsyncword(dev->spiport, dev->syncword);

    setpower(dev->spiport, dev->power);

    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(dev->spiport, REG_DIO_MAPPING_1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // clear all radio IRQ flags
    writeReg(dev->spiport, REG_IRQ_FLAGS, 0xFF);
    // mask all IRQs but TxDone
    writeReg(dev->spiport, REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    // initialize the payload size and address pointers
    writeReg(dev->spiport, REG_FIFO_TX_BASE_AD, 0x00); writeReg(dev->spiport, REG_FIFO_ADDR_PTR, 0x00);

    // write buffer to the radio FIFO
    int i;

    for (i = 0; i < size; i++) { 
        writeReg(dev->spiport, REG_FIFO, payload[i]);
    }

    writeReg(dev->spiport, REG_PAYLOAD_LENGTH, size);

    // now we actually start the transmission
    opmode(dev->spiport, OPMODE_TX);

    // wait for TX done
    while(digitalRead(dev->dio[0]) == 0);

    MSG_LOG(DEBUG_INFO, "\nINFO~Transmit at SF%iBW%ld on %.6lf.\n", dev->sf, (dev->bw)/1000, (double)(dev->freq)/1000000);

    // mask all IRQs
    writeReg(dev->spiport, REG_IRQ_FLAGS_MASK, 0xFF);

    // clear all radio IRQ flags
    writeReg(dev->spiport, REG_IRQ_FLAGS, 0xFF);

    // go from stanby to sleep
    opmode(dev->spiport, OPMODE_SLEEP);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int already_running(void)
{
    int     fd;
    char    buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        fprintf(stderr, "ERROR~can't open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return(1);
        }
        fprintf(stderr, "ERROR~can't lock %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);
    return(0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t bw_getval(int x) {
    switch (x) {
        case BW_500KHZ: return 500000;
        case BW_250KHZ: return 250000;
        case BW_125KHZ: return 125000;
        case BW_62K5HZ: return 62500;
        case BW_31K2HZ: return 31200;
        case BW_15K6HZ: return 15600;
        case BW_7K8HZ : return 7800;
        default: return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t sf_getval(int x) {
    switch (x) {
        case DR_LORA_SF7: return 7;
        case DR_LORA_SF8: return 8;
        case DR_LORA_SF9: return 9;
        case DR_LORA_SF10: return 10;
        case DR_LORA_SF11: return 11;
        case DR_LORA_SF12: return 12;
        default: return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t sf_toval(int x) {
    switch (x) {
        case 7: return DR_LORA_SF7; 
        case 8: return DR_LORA_SF8; 
        case 9: return DR_LORA_SF9;
        case 10: return DR_LORA_SF10;
        case 11: return DR_LORA_SF11;
        case 12: return DR_LORA_SF12;
        default: return -1;
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int32_t bw_toval(int x) {
    switch (x) {
        case 500000: return BW_500KHZ;
        case 250000: return BW_250KHZ;
        case 125000: return BW_125KHZ;
        case 62500: return BW_62K5HZ;
        case 31200: return BW_31K2HZ;
        case 15600: return BW_15K6HZ;
        case 7800: return BW_7K8HZ;
        default: return -1;
    }
}
