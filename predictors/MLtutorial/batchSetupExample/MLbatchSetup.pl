#!/usr/bin/perl -w

open(CLASSIFIERS, "classifiers.txt") || die;
my @classifiers = <CLASSIFIERS>;
close(CLASSIFIERS);
open(DATADISC, "dataDiscretizers.txt") || die;
my @datadisc = <DATADISC>;
close(DATADISC);
open(DATASETS, "datasets.txt") || die;
my @datasets = <DATASETS>;
close(DATASETS);

my $model = 1;
my $currPath = `pwd`;
chomp($currPath);
`mkdir $currPath/models`;
`mkdir $currPath/data`;
foreach $dataset (@datasets){
	chomp($dataset);

	my $dataPath = $dataset;
	$dataPath =~ s/([^\t]+)\t.+/$1/g;
    my $metaPath = $dataset;
    $metaPath =~ s/[^\t]+\t(.+)/$1/g;
    my $data = $dataset;
	$data =~ s/[^\t]+\/([^\t]+)\t.+/$1/g;
	my $meta = $dataset;
	$meta =~ s/[^\t]+\t.+\/(.+)/$1/g;

	foreach $datadisc (@datadisc){
		chomp($datadisc);
		my $dataDir = "$currPath/data/$data\_$datadisc";
		`mkdir $dataDir`;
		foreach $classifier (@classifiers){
			chomp($classifier);
			my $classifierType = $classifier;
			$classifierType =~ s/([^\t]+)\t.+/$1/g;
			my $classifierParams = $classifier;
            $classifierParams =~ s/[^\t]+\t(.+)/$1/g;

			open(CONFIG, ">models/model$model.cfg") || die;
			print CONFIG "name\tjob_1\n";
			print CONFIG "task\tdrug response\n";
			print CONFIG "inputType\tflatfiles\n";
			print CONFIG "trainingDir\t$dataDir/trainingDir\n";
			print CONFIG "validationDir\t$dataDir/validationDir\n";
			print CONFIG "modelDir\t$currPath/models/model$model\n";
			print CONFIG "crossValidation\tk-fold\n";
			print CONFIG "folds\t5\n";
            print CONFIG "outputType\t$classifierType\n";
			print CONFIG "classifier\t$classifierType\n";
			print CONFIG "parameters\t$classifierParams\n";
			print CONFIG "dataDiscretizer\t$datadisc\n";
			print CONFIG "clinDiscretizer\tNA\n";
			print CONFIG "dataFilepath\t$dataPath\n";
			print CONFIG  "metadataFilepath\t$metaPath\n";
			close(CONFIG);
	
			open(SETUP, ">>setupJobList.txt") || die;
			print SETUP "MLsetup models/model$model.cfg\n";
			close(SETUP);
            open(WRAPUP, ">>wrapupJobList.txt") || die;
			print WRAPUP "MLwriteResultsToRa models/model$model.cfg tasks.ra classifiers.ra subgroups.ra jobs.ra\n";	
			close(WRAPUP);

			$model++;
		}
	}
}
