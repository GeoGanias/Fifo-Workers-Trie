#!/bin/bash
declare -a array=();
declare -a intarray=();
declare -a filearray=();
k=0;
i=0;
for file in ./log/*
do
    filearray=();#mhdenizetai se ka8e loop arxeio wste na tsekarei an exoun     ksanatsekarei thn leksh sto arxeio
    while IFS='' read -r line || [[ -n "$line" ]]; do
        flag=0;
        foundkey=0;
        stopnum=0;
        for word in $line; do
            if [ "$word" = "Search" ] ; then
                flag=1;
            elif [ "$word" = ":" ] ; then
                stopnum=$((stopnum+1));
            elif [ "$flag" = 1 -a "$foundkey" = 0 ] ; then
                exists=0;
                exists_infile=0;
                for wrd in ${array[@]} ; do
                    if [ "$wrd" = $word ]; then
                        exists=1;
                    fi
                done
                for wrd in ${filearray[@]} ; do
                    if [ "$wrd" = $word ]; then
                        exists_infile=1;
                    fi
                done
                if [ "$exists" = 0 ]; then
                    array[$k]=$word;
                fi
                if [ "$exists_infile" = 0 ]; then
                    filearray[$k]=$word;
                fi
                foundkey=1;
            fi
        done
        if [ "$i" = 0 -a "$flag" = 1 -a "$exists" = 0 ] ; then
            intarray[$k]=$((stopnum-2));
            k=$((k+1));
        elif [ "$i" != 0  -a "$flag" = 1 -a "$exists" = 1 -a "$exists_infile" = 0 ] ; then #to exists 8a einai panta 1 an to i>0
            intarray[$k]=$((intarray[$k]+stopnum-2));
            k=$((k+1));
        fi



    done < "$file"
    i=$((i+1));
    k=0;
done
#anazhthsh gia min
min=${intarray[0]};
for w in ${intarray[@]} ; do
    if [ "$min" = 0 -a "$w" != 0 ]; then
        min=$w;
    fi
    if [ "$w" != 0 ]; then
        ((w < min )) && min=$w
    fi
done
#echo $min;
#arrlength=${#array[@]}
#for (( i=0; i<${arrlength}; i++)); do
#    echo "->> ${array[$i]} ${intarray[$i]}"
#done
i=0;
for w in ${intarray[@]} ; do
    if [ "$w" = $min ]; then
        word=${array[$i]};
        echo "$word [totalNumFilesFound: $min]"
    fi
    i=$((i+1));
done

