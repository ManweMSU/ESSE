mkdir resurrect/_obj
mkdir resurrect/_bin
echo 'Resurrectorem compilo...'
cc resurrect/resurrector.c -o resurrect/_bin/resurrector &>> resurrect/_obj/resurrector.log || { echo "Compilatio resurrectoris falsa."; exit 1; }
echo 'Constructorem resurgo...'
resurrect/_bin/resurrector resurrect/aux-stub.cxx \
    constructor/ \
    moduli/Auxilia/ \
    moduli/Auxilia-Linux/ \
    moduli/Codices/ \
    moduli/Codices-Linux/ \
    moduli/Communicatio/ \
    moduli/Communicatio-Linux/ \
    moduli/Compressio/ \
    moduli/Consolatorium/ \
    moduli/Consolatorium-Linux/ \
    moduli/Cor/ \
    moduli/Cor-Linux/ \
    moduli/DBus-Linux/ \
    moduli/Energia/ \
    moduli/Energia-Linux/ \
    moduli/Fenestrae/ \
    moduli/Fenestrae-Nullae/ \
    moduli/Formatio-Imaginum/ \
    moduli/Formationes/ \
    moduli/Graphica/ \
    moduli/Graphica-Linux/ \
    moduli/Imagines/ \
    moduli/EngineRuntime/ \
    I:moduli I:moduli/Graphica-Linux/api I:moduli/EngineRuntime J:resurrect/_obj O:resurrect/_bin/esse \
    &>> resurrect/_obj/resurrector.log || { echo "Resurrectio ESSE falsa."; exit 2; }
cp constructor/esse.loc.ini resurrect/_bin/esse.loc.ini
mkdir bin
resurrect/_bin/esse -R bin/esse.ini --reconfigura -N || { echo "Reconfiguratio falsa."; exit 3; }
resurrect/_bin/esse -R bin/esse.ini esse.ertproj -N || { echo "Compilatio ultima ESSE falsa."; exit 4; }
resurrect/_bin/esse -R bin/esse.ini convertor/esse-converte.esse -N || { echo "Compilatio ultima convertoris falsa."; exit 4; }
ESSE_CONS_EXEC=`resurrect/_bin/esse -R bin/esse.ini esse.ertproj -OSE`
ESSE_CONV_EXEC=`resurrect/_bin/esse -R bin/esse.ini convertor/esse-converte.esse -OSE`
cp resurrect/esse.loc.ini bin/esse.loc.ini
cp "$ESSE_CONS_EXEC" bin/esse
cp "$ESSE_CONV_EXEC" bin/esse-converte
mkdir bin/esse.loc
mkdir bin/esse.conv.loc
bin/esse-converte lineae constructor/esse.loc.en.txt -Nfo bin bin/esse.loc/en.ecst
bin/esse-converte lineae constructor/esse.loc.ru.txt -Nfo bin bin/esse.loc/ru.ecst
bin/esse-converte lineae convertor/loc.en.txt -Nfo bin bin/esse.conv.loc/en.ecst
bin/esse-converte lineae convertor/loc.ru.txt -Nfo bin bin/esse.conv.loc/ru.ecst
rm -rf resurrect/_obj
rm -rf resurrect/_bin
rm -rf _build
rm -rf convertor/_build
echo "Resurrectum bene est!"
bin/esse -NF