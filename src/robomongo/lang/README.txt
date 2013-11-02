Using:

    $ cd src/robomongo/lang
    $ ./ts_update.sh [all|<lang_code>] [lupdate params]
    
    < open robomongo_<lang_code>.raw.ts in Qt Linguist, translate and save >

    $ ./ts_compile.sh
    
    < take src/robomongo/lang/robomongo_*.qm and move it to target/install/lib/translations directory >

