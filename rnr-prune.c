#ifndef WIN32
#include <unistd.h>
#endif

#include <math.h>
#include <time.h> 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "Tree.h"
#include "List.h"
#include "BitVector.h"
#include "sharedVariables.h"
#include "newFunctions.h"

#define PROG_NAME "RnR-prune"
#define PROG_VERSION "0.1"
#define PROG_RELEASE_DATE "not yet"


extern char run_id[128];
extern char *workdir;
extern double masterTime;
extern int tid;
extern int NumberOfThreads; 
extern volatile int NumberOfJobs;

void pruneTaxaFromTreeset(char *bootstrapFileName, char *bestTreeFile, char *toDropFileName, All *tr)
{
  int 
    i;

  IndexList
    *iter;

  FILE 
    *toDrop = myfopen(toDropFileName, "r");
  
  /* drop taxa from bootstrap trees  */
  if( strcmp(bootstrapFileName, ""))
    {
      tr->tree_string = CALLOC(10 * getTreeStringLength(bootstrapFileName), sizeof(char));
      if  (NOT setupTree(tr, bootstrapFileName))
	{
	  PR("Something went wrong during tree initialisation. Sorry.\n");
	  exit(-1);
	}   
      FILE *TMP = getNumberOfTrees(tr, bootstrapFileName);
      fclose(TMP);
    }

  /* drop taxa from best-known tree */
  if( strcmp(bestTreeFile, ""))
    {
      tr->tree_string = CALLOC(10 * getTreeStringLength(bestTreeFile), sizeof(char));
      if  (NOT setupTree(tr, bestTreeFile))
	{
	  PR("Something went wrong during tree initialisation. Sorry.\n");
	  exit(-1);
	}   
      FILE *tmp = getNumberOfTrees(tr, bestTreeFile);
      fclose(tmp);
    }

  IndexList
    *indicesToDrop = parseToDrop(tr, toDrop);

  if( strcmp(bootstrapFileName, ""))
    {   
      FILE
	*bootstrapFile = getNumberOfTrees(tr, bootstrapFileName),
	*outf = getOutputFileFromString("prunedBootstraps");
      
      FOR_0_LIMIT(i,tr->numberOfTrees)
	{
	  readBootstrapTree(tr, bootstrapFile);

	  iter = indicesToDrop;
	  FOR_LIST(iter)
	    pruneTaxon(tr, iter->index);

	  char *tmp = writeTreeToString(tr , FALSE);	
	  fprintf(outf, "%s", tmp);
	}  
  
      fclose(outf);
    }

  if( strcmp(bestTreeFile, ""))
    {
      FILE 
	*bestTree = myfopen(bestTreeFile, "r"),
	*outf = getOutputFileFromString("prunedBestTree");
      
      readBestTree(tr, bestTree);
      iter = indicesToDrop;
      FOR_LIST(iter)
	pruneTaxon(tr, iter->index);
      
      char *tmp = writeTreeToString(tr, TRUE); 
      fprintf(outf, "%s", tmp);	  
    }
  
  freeIndexList(indicesToDrop);
  exit(EXIT_SUCCESS);
}

static void printHelpFile()
{
  printVersionInfo(FALSE);
  printf("This program prunes a list of taxa from a bootstrap tree or a single tree with branch lengths (such as a best-known ML/MP-tree).\n\nSYNTAX: ./%s [-i <bootTrees> | -t <treeFile>] -x <excludeFile> -n <runId> [-w <workingDir>] [-h]\n", lowerTheString(programName));
  printf("\n\tOBLIGATORY:\n");
  printf("-x <excludeFile>\n\tA list of taxa (one taxon per line) to prune from either the bootstrap trees or the single best-known tree.\n");
  printf("-i <bootTrees>\n\tA collection of bootstrap trees.\n");
  printf("-t <treeFile>\n\tA single tree with branch lengths. Use either this flag or the -i flag.\n");
  printf("-n <runId>\n\tAn identifier for this run.\n");  
  printf("\n\tOPTIONAL:\n");
  printf("-w <workDir>\n\tA working directory where output files are created.\n");
  printf("-h\n\tThis help file.\n");
}


int main(int argc, char *argv[])
{
  int
    c; 
  
  char
    *bestTreeFileName = "",
    *excludeFileName = "",
    *bootTreesFileName = "";

  programName = PROG_NAME; 
  programVersion = PROG_VERSION;
  programReleaseDate = PROG_RELEASE_DATE; 

  while((c = getopt(argc,argv, "hi:t:x:n:")) != -1)
    {
      switch(c)
	{
	case 'i':
	  bootTreesFileName = optarg;
	  break;
	case 'w':
	  strcpy(workdir,optarg);
	  break;
	case 'n':
	  strcpy(run_id,optarg);
	  break;
	case 't':
	  bestTreeFileName = optarg;
	  break;
	case 'x':	  
	  excludeFileName = optarg;
	  break;
	case 'h': 
	default:
	  {
	    printHelpFile();
	    abort();
	  }
	}
    }

  if( NOT (strcmp(bootTreesFileName, "") ||  strcmp(bestTreeFileName, "")))
    {
      printf("Please specify either a set of bootstrap trees (-i) or a single best-known tree (-t) from which you want to prune taxa.\n");
      printHelpFile();
      exit(-1);
    }

  if( NOT strcmp(excludeFileName, "") )
    {
      printf("Please specify a file containing taxa to prune (one taxon a line) via -x\n");
      printHelpFile();
      exit(-1);
    }

  if ( NOT strcmp(run_id, ""))
    {
      printf("Please specify a run id via -n.\n");
      printHelpFile();
      exit(-1);
    }

  compute_bits_in_16bits();
  initializeMask();	
  
  setupInfoFile();
   
  All *tr = CALLOC(1,sizeof(All));
  tr->numBranches = 1;
  pruneTaxaFromTreeset(bootTreesFileName, bestTreeFileName, excludeFileName, tr);

  return 0;
}
