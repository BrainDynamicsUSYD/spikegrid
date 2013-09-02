echo input/* | tr ' ' '\n' | parallel --eta 'mono --optimize=all classify.exe {} && mono --optimize=all analysis.exe {}'
