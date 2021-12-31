BEGIN { 
	print "\\begin{tabular}{|l|r|l|l|c|l|l|l|}";
	print "Type & Dim & DSP Variable & Signal Name & Unit & Conversion Factor & Module & Signal Description \\\\ \\hline";
	FS="[,]"
}
{
	if (/MAKE_DSP_SIG_ENTRY \(/){
		gsub(/MAKE_DSP_SIG_ENTRY \(/,",")
		gsub(/\"\),/,",")
		gsub(/_/,"\\_")
		gsub(/^/,"\\^")
		gsub(/"/,"")
		printf "SI32 & 1 "
		for (i = 2; i <= NF-1; i++)
			printf "& %s ", $i;
		print "\\\\"; 
	} else if (/MAKE_DSP_SIG_ENTRY_VECTOR \(/){
		gsub(/MAKE_DSP_SIG_ENTRY_VECTOR \(/,",")
		gsub(/\"\),/,",")
		gsub(/_/,"\\_")
		printf "VI32 "
		for (i = 2; i <= NF-1; i++)
			printf "& %s ", $i;
		print "\\\\"; 
	}
}
END { 
	print "\\hline";
	print "\\end{tabular}" 
}

