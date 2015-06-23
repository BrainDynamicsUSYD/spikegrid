INFERANALYZE= /usr/lib/infer/infer/infer/bin/InferAnalyze $(INFER_OPTIONS) -results_dir '/data/conductance/infer-out'

OBJECTS=cl1 

default: test

all: test

test: $(OBJECTS)
	echo "Analysis done"

cl1 : 
	$(INFERANALYZE) -cluster xcl1.cluster >cl1


clean:
	rm -f $(OBJECTS)
