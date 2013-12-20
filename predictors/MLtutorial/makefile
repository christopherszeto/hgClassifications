test1::
	MLsetup example1.cfg
	chmod u+x trainingJobList.txt
	./trainingJobList.txt
	chmod u+x validationJobList.txt
	./validationJobList.txt

test2::
	MLsetup example2.cfg
	chmod u+x trainingJobList.txt
	./trainingJobList.txt
	chmod u+x validationJobList.txt
	./validationJobList.txt

clean:: 
	rm -rf modelDir trainingDir validationDir *validationJobList.txt *trainingJobList.txt trans_predictions
	rm -rf tasks.ra classifiers.ra subgroups.ra jobs.ra
