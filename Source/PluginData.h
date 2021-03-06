/**
 *
 * Copyright (c) 2014-2015 Pascal Gauthier.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 */

#ifndef PLUGINDATA_H_INCLUDED
#define PLUGINDATA_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#define SYSEX_SIZE 4104

#include <stdint.h>
#include "Dexed.h"

uint8_t sysexChecksum(const uint8_t *sysex, int size);
void exportSysexPgm(uint8_t *dest, uint8_t *src);

#define SYSEX_HEADER { 0xF0, 0x43, 0x00, 0x00, 0x20, 0x00 }

class Cartridge {
    uint8_t voiceData[SYSEX_SIZE];
    uint8_t perfData[SYSEX_SIZE];
    
    void setHeader() {
        uint8 voiceHeader[] = SYSEX_HEADER;
        memcpy(voiceData, voiceHeader, 6);
        voiceData[4102] = sysexChecksum(voiceData+6, 4096);
        voiceData[4103] = 0xF7;
    }
    
public:
    Cartridge() { }
    
    Cartridge(const Cartridge &cpy) {
        memcpy(voiceData, cpy.voiceData, SYSEX_SIZE);
        memcpy(perfData, cpy.perfData, SYSEX_SIZE);
    }
    
    static String normalizePgmName(const char *sysexName) {
        char buffer[11];
        
        memcpy(buffer, sysexName, 10);
        
        for (int j = 0; j < 10; j++) {
            char c = (unsigned char) buffer[j];
            switch (c) {
                case 92:
                    c = 'Y';
                    break; /* yen */
                case 126:
                    c = '>';
                    break; /* >> */
                case 127:
                    c = '<';
                    break; /* << */
                default:
                    if (c < 32 || c > 127)
                        c = 32;
                    break;
            }
            buffer[j] = c;
        }
        buffer[10] = 0;
        
        return String(buffer);
    }
    
    int load(File f) {
        FileInputStream *fis = f.createInputStream();
        if ( fis == NULL )
            return -1;
        int rc = load(*fis);
        delete fis;
        return rc;
    }
    
    int load(InputStream &fis) {
        uint8 buffer[65535];
        int sz = fis.read(buffer, 65535);
        if ( sz == 0 )
            return -1;
        return load(buffer, sz);
    }
    
    int load(const uint8_t *stream, int size) {
        uint8 voiceHeader[] = SYSEX_HEADER;
        uint8 tmp[65535];
        uint8 *pos = tmp;
        int status = 3;
        
        if ( size > 65535 )
            size = 65535;
        
        memcpy(tmp, stream, size);
        
        while(size >= 4104) {
            // random data
            if ( pos[0] != 0xF0 ) {
                if ( status != 0 )
                    return status;
                memcpy(voiceData, pos+6, 4096);
                return 2;
            }
            
            pos[3] = 0;
            if ( memcmp(pos, voiceHeader, 6) == 0 ) {
                memcpy(voiceData, pos, SYSEX_SIZE);
                if ( sysexChecksum(voiceData + 6, 4096) == pos[4102] )
                    status = 0;
                else
                    status = 1;
                size -= 4104;
                pos += 4104;
                continue;
            }

            // other sysex
            int i;
            for(i=0;i<size-1;i++) {
                if ( pos[i] == 0xF7 )
                    break;
            }
            size -= i;
            stream += i;
        }
        
        return status;
    }
    
    int saveVoice(File f) {
        setHeader();
        return f.replaceWithData(voiceData, SYSEX_SIZE);
    }
    
    void saveVoice(uint8_t *sysex) {
        setHeader();
        memcpy(sysex, voiceData, SYSEX_SIZE);
    }
    
    char *getRawVoice() {
        return (char *) voiceData + 6;
    }
    
    char *getVoiceSysex() {
        setHeader();
        return (char *) voiceData;
    }
    
    void getProgramNames(StringArray &dest) {
        dest.clear();
        for (int i = 0; i < 32; i++)
            dest.add( normalizePgmName(getRawVoice() + ((i * 128) + 118)) );
    }
    
    Cartridge operator =(const Cartridge other) {
        memcpy(voiceData, other.voiceData, SYSEX_SIZE);
        memcpy(perfData, other.perfData, SYSEX_SIZE);
        return *this;
    }
    
    void unpackProgram(uint8_t *unpackPgm, int idx);
    void packProgram(uint8_t *src, int idx, String name);
};

#endif  // PLUGINDATA_H_INCLUDED
