//
// General macro to configure the RSN analysis task.
// It calls all configs desired by the user, by means
// of the boolean switches defined in the first lines.
// ---
// Inputs:
//  1) flag to know if running on MC or data
//  2) path where all configs are stored
// ---
// Returns:
//  kTRUE  --> initialization successful
//  kFALSE --> initialization failed (some config gave errors)
//

Bool_t usePhi   = 1;
Bool_t useKStar = 1;
Bool_t usePhiRAA = 1;

 //set to kTRUE if using data AOD049 - needed to enable centrality patch
Bool_t isAOD049 = 0;

AliRsnMiniAnalysisTask * AddAnalysisTaskRsnMini
(
   Bool_t      isMC,
   Bool_t      isPP,
   const char *path,
   Int_t       nmix = 0
)
{  
   //
   // -- INITIALIZATION ----------------------------------------------------------------------------
   //
   
   // retrieve analysis manager
   AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();

   // create the task and connect with physics selection
   AliRsnMiniAnalysisTask *task = new AliRsnMiniAnalysisTask("RSN", isMC);
   if (isAOD049 && !isMC && !isPP){
     task->SetUseCentralityPatch(kTRUE);
   }
   mgr->AddTask(task);
   
   // settings
   if (isPP) 
      task->UseMultiplicity("QUALITY");
   else
      task->UseCentrality("V0M");
   
   // set mixing
   task->UseContinuousMix();
   //task->UseBinnedMix();
   task->SetNMix(nmix);
   task->SetMaxDiffVz(1.0);
   task->SetMaxDiffMult(10.0);
   task->SetMaxDiffAngle(1E20);
   
   //
   // -- EVENT CUTS (same for all configs) ---------------------------------------------------------
   //
   
   // cut on primary vertex:
   // - 2nd argument --> |Vz| range
   // - 3rd argument --> minimum required number of contributors
   // - 4th argument --> tells if TPC stand-alone vertexes must be accepted
   AliRsnCutPrimaryVertex *cutVertex = new AliRsnCutPrimaryVertex("cutVertex", 10.0, 0, kFALSE);
   
   // set the check for pileup
   if (isPP) cutVertex->SetCheckPileUp(kTRUE);
      
   // define and fill cut set
   AliRsnCutSet *eventCuts = new AliRsnCutSet("eventCuts", AliRsnTarget::kEvent);
   eventCuts->AddCut(cutVertex);
   eventCuts->SetCutScheme(cutVertex->GetName());
   
   // set cuts in task
   task->SetEventCuts(eventCuts);
   
   //
   // -- EVENT-ONLY COMPUTATIONS -------------------------------------------------------------------
   //
   
   // initialize value computation for multiplicity/centrality
   // second argument tells if the value must be taken from MC
   // (when this can be done)
   // after creating the value, the task returns its ID
   Int_t multID = task->CreateValue(AliRsnMiniValue::kMult, kFALSE);
   
   // create event-related output
   AliRsnMiniOutput *outMult = task->CreateOutput("eventMult", "HIST", "EVENT");
   // set axes, by passing value ID and defining the binning
   if (isPP) 
      outMult->AddAxis(multID, 300, 0.0, 300.0);
   else
      outMult->AddAxis(multID, 100, 0.0, 100.0);
   
   //
   // -- PAIR CUTS (common to all resonances) ------------------------------------------------------
   //
   
   AliRsnCutMiniPair *cutY = new AliRsnCutMiniPair("cutRapidity", AliRsnCutMiniPair::kRapidityRange);
   cutY->SetRangeD(-0.5, 0.5);
   
   AliRsnCutSet *cutsPair = new AliRsnCutSet("pairCuts", AliRsnTarget::kMother);
   cutsPair->AddCut(cutY);
   cutsPair->SetCutScheme(cutY->GetName());
   
   //
   // -- CONFIGS -----------------------------------------------------------------------------------
   //
   
   if (usePhiRAA){
	   if (!isMC) {
	      gROOT->LoadMacro(Form("%s/ConfigPhiRAApp.C", path));
	      if (!ConfigPhiRAApp(task, isMC, isPP, "", cutsPair)) return 0x0;
	   }
   }

   if (usePhi) {
      if (isPP) {
         gROOT->LoadMacro(Form("%s/ConfigPhi.C", path));
         if (!ConfigPhi(task, isMC, "", cutsPair)) return 0x0;
      } else {
         gROOT->LoadMacro(Form("%s/ConfigPhiPbPb.C", path));
         if (!ConfigPhiPbPb(task, isMC, "", cutsPair)) return 0x0;
      }
      if (isMC) {
         gROOT->LoadMacro(Form("%s/ConfigPhiMC.C", path));
         if (!ConfigPhiMC(task, isPP, "", cutsPair)) return 0x0;
      }
   }
   
   if (useKStar) {
      gROOT->LoadMacro(Form("%s/ConfigKStar.C", path));
      if (!ConfigKStar(task, isMC, "", cutsPair)) return 0x0;
      if (isMC) {
         gROOT->LoadMacro(Form("%s/ConfigKStarMC.C", path));
         if (!ConfigKStarMC(task, isPP, "", cutsPair)) return 0x0;
      }
   }
   
   //
   // -- CONTAINERS --------------------------------------------------------------------------------
   //
   
   const char *file = AliAnalysisManager::GetCommonFileName();
   AliAnalysisDataContainer *output = mgr->CreateContainer("RsnOut", TList::Class(), AliAnalysisManager::kOutputContainer, file);
   mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer());
   mgr->ConnectOutput(task, 1, output);

   return task;
}
