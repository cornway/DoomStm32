                    PRESERVE8

                    EXPORT upc

                    AREA    |.text|, CODE, READONLY

upc                 FUNCTION
                    SWI     0x02
                    BX      LR
                    ENDP
                        
                    END