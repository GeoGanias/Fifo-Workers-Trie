# Fifo-Workers-Trie
<b>Εργασια 2 Προγραμματισμός Συστημάτων / Systems Programming assignment 2</b>

    make
    ./jobExecutor -d docfile -w N
Αρχικά το πρόγραμμα παίρνει σαν ορίσματα το docfile που περιέχει paths απο φακέλους. π.χ:   
   dir1     
   /home/tmp/dir2   
   tempdir/dir3     

κλπ.
και τον αριθμό των workers που θα φτιάξει. Φτιάχνει 2 fifo για κάθε worker, (1 "rfi" για να στελνει μηνύματα προς τον jobExecutor και 1 "fi" για να δέχεται μηνύματα απο τον jobExecutor, οπου i=0,1,2,...,N) και τις ανοιγει μολις τις φτιάξει, και τις κλείνουμε στο τέλος του προγραμματος.

## jobExecutor:
Μετά το άνοιγμα των fifo ο jobExecutor ανοιγει το αρχείο docfile και μοιράζει τα αρχεία στους workers (1ο αρχείο στον 1ο Worker, 2o αρχείο στον 2ο Worker κλπ.) κάνοντας write το path του αρχείου στην κατάλληλη fifo.

Ταυτόχρονα αποθηκεύει σε μια δομη (Allpaths) την πληροφορία για το ποιος Worker έχει ποιά paths.

Όταν τελειώσει με το να στέλνει αρχεία, στέλνει ένα τελευταίο μήνυμα οτι τελείωσε (στέλνει 0) περιμένει την απάντηση απο τους Workers οτι τελείωσαν (κανει read το -1) με την διαδικασία επεξεργασίας των κειμένων που τους δώθηκαν.

Στην συνέχεια περιμένει την εντολή από τον χρήστη και ανάλογα στέλνει μεσω του fifo στους workers την ανάλογη εντολή καθώς και τις πληροφορίες που χρειάζονται για αυτη, και περιμένει την απάντησή τους.

Κάθε φορά πριν σταλθεί κάποια εντολή ελέγχουμε αν εχει γίνει kill κάποιος worker, μέσω ενός πίνακα με τα process ids των Workers. Αν γίνει kill ένας Worker αλλάζουμε το process id στον πινακα με -1 και αν στον έλεγχο βρούμε το -1 δημιουργουμε καινούριο Worker ο οποίος θα διαχειρίζεται τα ίδια paths με αυτόν που τερμάτησε.

## Worker:
Ο Worker διαβάζει τα paths των αρχείων που στέλνει ο job executor και τα αποθηκεύει σε μια δομή (Pathslist) η οποία χρησιμοποιείται στην εντολη /wc. και για την δημιουργία του trie.
Αφου διαβάσει τα paths δημιουργει το trie (το οποίο έχει παρθεί απο την 1η εργασία αλλά άλλαξα το posting list) και στέλνει μήνυμα στον jobExecutor (-1) οτι τελείωσε.

αλλαγη posting_list: Πλεόν κρατάει το path και μια απλά συνδ. λίστα από (text_id, times_shown), όπου text_id= ο αριθμός της γραμμής στο συγκεκριμένο αρχείο στο οποίο βρέθηκε η λέξη , και times_shown= πόσες φορές βρέθηκε η λέξη στην συγκεκριμένη γραμμή

Μετά περιμένει να διαβάσει απο το fifo την εντολή που έγραψε ο jobExecutor μεχρι να δεχθεί το exit.

## Commands:

    /search
Ο κάθε Worker δέχεται το query ολόκληρο (μαζί με το -d deadline) και διαβάζει το deadline που έχει.
Κάνει search στο trie και αποθηκεύει σε μια δομή (ιδια με αυτη του posting list) τα αποτελέσματα από όλες τις λέξεις που είχε να βρει (paths και αριθμούς γραμμής). Τσεκάρει αν έχει περάσει το χρονικό περιθώριο deadline, και αν δεν έχει περάσει τότε στέλνει τα αποτελέσματα, αλλίως μήνυμα αποτυχίας. ο jobExecutor τα τυπώνει στον χρήστη, και με την βοήθεια της συνάρτησης findText (η οποία παιρνει σαν ορίσματα path και αριθμό γραμμής), εκτυπώνει την πρόταση.

    /maxcount
ο κάθε worker ψάχνει στο trie την λέξη που του δώθηκε. Όταν την βρει ψάχνει σε ποιό αρχείο βρέθηκε τις περισσοτερες φορές, και επιστρέφει το max του. Ο jobExecutor συγκρίνει τα max και επιστρέφει στον χρήστη το μεγαλύτερο.

    /mincount 
Ιδια λογική με το maxcount.

    /wc
κάθε worker αθροιζει τα bytes, τις λέξεις και τις γραμμές απο τα κείμενα που έχει και τα στέλνει στον jobExecutor. Αυτος κάνει ένα γενικό αθροισμα και τα τυπώνει στον χρήστη. 

## Bash Scripts

Τα Worker_logs αποθηκεύονται σε φάκελο log/ και είναι της μορφής:   
Time : Search : word1 : path1 : path2   
Time : Search : word_which_doesnt_exist     
Time : Maxcount : word : path1 : path2      
Time : Mincount : word : path1 : path2      
Time : WC : Bytes num1 : Total_Words num2 : Total_Lines num3    
Time : Search : word : path1     
Time : Search : word2    
Timeout     
Time : Search : word    
κλπ (Το Timeout είναι αν δεν προλάβει μεσα σε χρόνο -d να εκτελέσει ένα search, όμως γράφει τα search που πρόλαβε να κάνει ο worker πριν τελειώσει ο χρονος)

Πάνω σε αυτά τα logs εγιναν τα bash scripts:

    bash1.sh (Total Number of keywords searched)
Για κάθε αρχείο στον φάκελο log/ διαβάζουμε γραμμη γραμμη και εισάγουμε σε έναν πίνακα τις λέξεις που έχουν γίνει search (array) και σε έναν άλλο πίνακα σε ποσα αρχεία βρέθηκε αυτή η λέξη (intarray). Για αυτό έχουμε έναν βοηθητικό πίνακα (filearray), o οποίος είναι ιδιος με τον array ομως τον αδειάζω κάθε φορα που εξετάζουμε καινουριο log αρχείο.

Στο τέλος έχουμε έναν τον πίνακα με τις διαφορετικές λέξεις και τον πίνακα με το σε πόσα αρχεία βρέθηκαν.
Άρα αθροίζουμε σε έναν μετρητή τον αριθμό των λέξεων οι οποίες βρέθηκαν τουλάχιστον 1 φορα σε αρχείο.

    bash2.sh (Keyword most frequently found)
Κάνουμε ακριβώς το ίδιο αλλά στο τέλος βρίσκουμε το max απο τον πίνακα που περιέχει το πόσες φορές βρέθηκε.
Εμφανίζουμε όλες τις λέξεις που βρέθηκαν σε max αρχεία.

    bash3.sh (Keyword least frequently found)
Κάνουμε το ίδιο με το max απλά βρίσκουμε το min (που βρέθηκε τουλάχιστον σε 1 αρχείο)
