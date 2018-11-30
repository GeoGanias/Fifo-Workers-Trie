#!/bin/bash
declare -a array=();
declare -a intarray=();
declare -a filearray=();
k=0;
i=0;
for file in ./log/*
do
    filearray=();#mhdenizetai se ka8e loop arxeio wste na tsekarei an exoun ksanatsekarei thn leksh sto arxeio
    while IFS='' read -r line || [[ -n "$line" ]]; do
        flag=0;
        foundkey=0;
        stopnum=0;
        for word in $line; do
            if [ "$word" = "Search" ] ; then #an flag!=1 den eimaste se search
                flag=1;
            elif [ "$word" = ":" ] ; then
                stopnum=$((stopnum+1)); #stopnum exei ta paths sta opoia bre8hke h leksh (+2)
            elif [ "$flag" = 1 -a "$foundkey" = 0 ] ; then #mpainei 1 fora se ka8e grammh edw logw ths foundkey
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
            intarray[$k]=$((stopnum-2))
            k=$((k+1));
        elif [ "$i" != 0  -a "$flag" = 1 -a "$exists" = 1 -a "$exists_infile" = 0 ] ; then #to exists 8a einai panta 1 an to i>0
            intarray[$k]=$((intarray[$k]+stopnum-2));#update tis upoloipes fores
            k=$((k+1));
        fi



    done < "$file"
    i=$((i+1));
    k=0;
done
#arrlength=${#array[@]}
#for (( i=0; i<${arrlength}; i++)); do
#    echo "->> ${array[$i]} ${intarray[$i]}"
#done

#a8roisma keywords pou bre8hkan
counter=0;
for w in ${intarray[@]} ; do
    if [ "$w" != 0 ]; then
        counter=$((counter+1))
    fi
done

echo "Number Of Keywords searched: $counter"
