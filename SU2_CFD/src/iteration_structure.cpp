/*!
 * \file iteration_structure.cpp
 * \brief Main subroutines used by SU2_CFD
 * \author F. Palacios, T. Economon
 * \version 6.2.0 "Falcon"
 *
 * The current SU2 release has been coordinated by the
 * SU2 International Developers Society <www.su2devsociety.org>
 * with selected contributions from the open-source community.
 *
 * The main research teams contributing to the current release are:
 *  - Prof. Juan J. Alonso's group at Stanford University.
 *  - Prof. Piero Colonna's group at Delft University of Technology.
 *  - Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *  - Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *  - Prof. Rafael Palacios' group at Imperial College London.
 *  - Prof. Vincent Terrapon's group at the University of Liege.
 *  - Prof. Edwin van der Weide's group at the University of Twente.
 *  - Lab. of New Concepts in Aeronautics at Tech. Institute of Aeronautics.
 *
 * Copyright 2012-2019, Francisco D. Palacios, Thomas D. Economon,
 *                      Tim Albring, and the SU2 contributors.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/iteration_structure.hpp"

CIteration::CIteration(CConfig *config) {
  rank = SU2_MPI::GetRank();
  size = SU2_MPI::GetSize();

  nInst = config->GetnTimeInstances();
  nZone = config->GetnZone();

  multizone = config->GetMultizone_Problem();
  singlezone = !(config->GetMultizone_Problem());

}

CIteration::~CIteration(void) { }

void CIteration::SetGrid_Movement(CGeometry **geometry,
          CSurfaceMovement *surface_movement,
          CVolumetricMovement *grid_movement,
          CSolver ***solver,
          CConfig *config,
          unsigned long IntIter,
          unsigned long ExtIter)   {

  unsigned short iDim;
  unsigned short Kind_Grid_Movement = config->GetKind_GridMovement();
  unsigned long nIterMesh;
  unsigned long iPoint;
  bool stat_mesh = true;
  bool adjoint = config->GetContinuous_Adjoint();
  bool discrete_adjoint = config->GetDiscrete_Adjoint();

  /*--- Only write to screen if this option is enabled ---*/
  bool Screen_Output = config->GetDeform_Output();
  
  unsigned short val_iZone = config->GetiZone();

  /*--- Perform mesh movement depending on specified type ---*/
  switch (Kind_Grid_Movement) {

  case RIGID_MOTION:

      if (rank == MASTER_NODE) {
        cout << endl << " Performing rigid mesh transformation." << endl;
      }

      /*--- Move each node in the volume mesh using the specified type
       of rigid mesh motion. These routines also compute analytic grid
       velocities for the fine mesh. ---*/

      grid_movement->Rigid_Translation(geometry[MESH_0],
                                       config, val_iZone, ExtIter);
      grid_movement->Rigid_Plunging(geometry[MESH_0],
                                    config, val_iZone, ExtIter);
      grid_movement->Rigid_Pitching(geometry[MESH_0],
                                    config, val_iZone, ExtIter);
      grid_movement->Rigid_Rotation(geometry[MESH_0],
                                    config, val_iZone, ExtIter);

      /*--- Update the multigrid structure after moving the finest grid,
       including computing the grid velocities on the coarser levels. ---*/

      grid_movement->UpdateMultiGrid(geometry, config);

      break;


    case ELASTICITY:

      if (ExtIter != 0) {

        if (rank == MASTER_NODE)
          cout << " Deforming the grid using the Linear Elasticity solution." << endl;

        /*--- Update the coordinates of the grid using the linear elasticity solution. ---*/
        for (iPoint = 0; iPoint < geometry[MESH_0]->GetnPoint(); iPoint++) {

          su2double *U_time_nM1 = solver[MESH_0][FEA_SOL]->node[iPoint]->GetSolution_time_n1();
          su2double *U_time_n   = solver[MESH_0][FEA_SOL]->node[iPoint]->GetSolution_time_n();

          for (iDim = 0; iDim < geometry[MESH_0]->GetnDim(); iDim++)
            geometry[MESH_0]->node[iPoint]->AddCoord(iDim, U_time_n[iDim] - U_time_nM1[iDim]);

        }

      }

      break;

 	/*--- Already initialized in the static mesh movement routine at driver level. ---*/ 
    case STEADY_TRANSLATION: case ROTATING_FRAME:
      break;

  }
  
  if (config->GetSurface_Movement(DEFORMING)){
      if (rank == MASTER_NODE)
        cout << endl << " Updating surface positions." << endl;

      /*--- Translating ---*/

      /*--- Compute the new node locations for moving markers ---*/

      surface_movement->Surface_Translating(geometry[MESH_0],
                                            config, ExtIter, val_iZone);
      /*--- Deform the volume grid around the new boundary locations ---*/

      if (rank == MASTER_NODE)
        cout << " Deforming the volume grid." << endl;
      grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                           config, true);

      /*--- Plunging ---*/

      /*--- Compute the new node locations for moving markers ---*/

      surface_movement->Surface_Plunging(geometry[MESH_0],
                                         config, ExtIter, val_iZone);
      /*--- Deform the volume grid around the new boundary locations ---*/

      if (rank == MASTER_NODE)
        cout << " Deforming the volume grid." << endl;
      grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                           config, true);

      /*--- Pitching ---*/

      /*--- Compute the new node locations for moving markers ---*/

      surface_movement->Surface_Pitching(geometry[MESH_0],
                                         config, ExtIter, val_iZone);
      /*--- Deform the volume grid around the new boundary locations ---*/

      if (rank == MASTER_NODE)
        cout << " Deforming the volume grid." << endl;
      grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                           config, true);

      /*--- Rotating ---*/

      /*--- Compute the new node locations for moving markers ---*/

      surface_movement->Surface_Rotating(geometry[MESH_0],
                                         config, ExtIter, val_iZone);
      /*--- Deform the volume grid around the new boundary locations ---*/

      if (rank == MASTER_NODE)
        cout << " Deforming the volume grid." << endl;
      grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                           config, true);

      /*--- Update the grid velocities on the fine mesh using finite
       differencing based on node coordinates at previous times. ---*/

      if (!adjoint) {
        if (rank == MASTER_NODE)
          cout << " Computing grid velocities by finite differencing." << endl;
        geometry[MESH_0]->SetGridVelocity(config, ExtIter);
      }

      /*--- Update the multigrid structure after moving the finest grid,
       including computing the grid velocities on the coarser levels. ---*/

      grid_movement->UpdateMultiGrid(geometry, config);

      }

  if (config->GetSurface_Movement(AEROELASTIC) 
      || config->GetSurface_Movement(AEROELASTIC_RIGID_MOTION)){

      /*--- Apply rigid mesh transformation to entire grid first, if necessary ---*/
      if (IntIter == 0) {
        if (Kind_Grid_Movement == AEROELASTIC_RIGID_MOTION) {

          if (rank == MASTER_NODE) {
            cout << endl << " Performing rigid mesh transformation." << endl;
          }

          /*--- Move each node in the volume mesh using the specified type
           of rigid mesh motion. These routines also compute analytic grid
           velocities for the fine mesh. ---*/

          grid_movement->Rigid_Translation(geometry[MESH_0],
                                           config, val_iZone, ExtIter);
          grid_movement->Rigid_Plunging(geometry[MESH_0],
                                        config, val_iZone, ExtIter);
          grid_movement->Rigid_Pitching(geometry[MESH_0],
                                        config, val_iZone, ExtIter);
          grid_movement->Rigid_Rotation(geometry[MESH_0],
                                        config, val_iZone, ExtIter);

          /*--- Update the multigrid structure after moving the finest grid,
           including computing the grid velocities on the coarser levels. ---*/

          grid_movement->UpdateMultiGrid(geometry, config);
        }

      }

      /*--- Use the if statement to move the grid only at selected dual time step iterations. ---*/
      else if (IntIter % config->GetAeroelasticIter() == 0) {

        if (rank == MASTER_NODE)
          cout << endl << " Solving aeroelastic equations and updating surface positions." << endl;

        /*--- Solve the aeroelastic equations for the new node locations of the moving markers(surfaces) ---*/

        solver[MESH_0][FLOW_SOL]->Aeroelastic(surface_movement, geometry[MESH_0], config, ExtIter);

        /*--- Deform the volume grid around the new boundary locations ---*/

        if (rank == MASTER_NODE)
          cout << " Deforming the volume grid due to the aeroelastic movement." << endl;
        grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                             config, true);

        /*--- Update the grid velocities on the fine mesh using finite
         differencing based on node coordinates at previous times. ---*/

        if (rank == MASTER_NODE)
          cout << " Computing grid velocities by finite differencing." << endl;
        geometry[MESH_0]->SetGridVelocity(config, ExtIter);

        /*--- Update the multigrid structure after moving the finest grid,
         including computing the grid velocities on the coarser levels. ---*/

        grid_movement->UpdateMultiGrid(geometry, config);
      }
        }
  if (config->GetSurface_Movement(FLUID_STRUCTURE)){
      if (rank == MASTER_NODE && Screen_Output)
        cout << endl << "Deforming the grid for Fluid-Structure Interaction applications." << endl;

      /*--- Deform the volume grid around the new boundary locations ---*/

      if (rank == MASTER_NODE && Screen_Output)
        cout << "Deforming the volume grid." << endl;
      grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                           config, true, false);

      nIterMesh = grid_movement->Get_nIterMesh();
      stat_mesh = (nIterMesh == 0);

      if (!adjoint && !stat_mesh) {
        if (rank == MASTER_NODE && Screen_Output)
          cout << "Computing grid velocities by finite differencing." << endl;
        geometry[MESH_0]->SetGridVelocity(config, ExtIter);
      }
      else if (stat_mesh) {
          if (rank == MASTER_NODE && Screen_Output)
            cout << "The mesh is up-to-date. Using previously stored grid velocities." << endl;
      }

      /*--- Update the multigrid structure after moving the finest grid,
       including computing the grid velocities on the coarser levels. ---*/

      grid_movement->UpdateMultiGrid(geometry, config);

  }
  if (config->GetSurface_Movement(FLUID_STRUCTURE_STATIC)){

    if ((rank == MASTER_NODE) && (!discrete_adjoint) && Screen_Output)
        cout << endl << "Deforming the grid for static Fluid-Structure Interaction applications." << endl;

      /*--- Deform the volume grid around the new boundary locations ---*/

    if ((rank == MASTER_NODE) && (!discrete_adjoint)&& Screen_Output)
        cout << "Deforming the volume grid." << endl;

      grid_movement->SetVolume_Deformation_Elas(geometry[MESH_0],
                                                                    config, true, false);

    if ((rank == MASTER_NODE) && (!discrete_adjoint)&& Screen_Output)
        cout << "There is no grid velocity." << endl;

      /*--- Update the multigrid structure after moving the finest grid,
       including computing the grid velocities on the coarser levels. ---*/

      grid_movement->UpdateMultiGrid(geometry, config);

  }
  if (config->GetSurface_Movement(EXTERNAL) || config->GetSurface_Movement(EXTERNAL_ROTATION)){
    /*--- Apply rigid rotation to entire grid first, if necessary ---*/

    if (Kind_Grid_Movement == EXTERNAL_ROTATION) {
      if (rank == MASTER_NODE)
        cout << " Updating node locations by rigid rotation." << endl;
      grid_movement->Rigid_Rotation(geometry[MESH_0],
                                                          config, val_iZone, ExtIter);
    }

    /*--- Load new surface node locations from external files ---*/
    
      if (rank == MASTER_NODE)
      cout << " Updating surface locations from file." << endl;
    surface_movement->SetExternal_Deformation(geometry[MESH_0],
                                                         config, val_iZone, ExtIter);

    /*--- Deform the volume grid around the new boundary locations ---*/
    
    if (rank == MASTER_NODE)
      cout << " Deforming the volume grid." << endl;
    grid_movement->SetVolume_Deformation(geometry[MESH_0],
                                                               config, true);
    
    /*--- Update the grid velocities on the fine mesh using finite
       differencing based on node coordinates at previous times. ---*/
    
    if (!adjoint) {
      if (rank == MASTER_NODE)
        cout << " Computing grid velocities by finite differencing." << endl;
      geometry[MESH_0]->SetGridVelocity(config, ExtIter);
  }

    /*--- Update the multigrid structure after moving the finest grid,
       including computing the grid velocities on the coarser levels. ---*/
    
    grid_movement->UpdateMultiGrid(geometry, config);
     
  }
}

void CIteration::Preprocess(COutput *output,
                            CIntegration ****integration,
                            CGeometry ****geometry,
                            CSolver *****solver,
                            CNumerics ******numerics,
                            CConfig **config,
                            CSurfaceMovement **surface_movement,
                            CVolumetricMovement ***grid_movement,
                            CFreeFormDefBox*** FFDBox,
                            unsigned short val_iZone,
                            unsigned short val_iInst) { }
void CIteration::Iterate(COutput *output,
                         CIntegration ****integration,
                         CGeometry ****geometry,
                         CSolver *****solver,
                         CNumerics ******numerics,
                         CConfig **config,
                         CSurfaceMovement **surface_movement,
                         CVolumetricMovement ***grid_movement,
                         CFreeFormDefBox*** FFDBox,
                         unsigned short val_iZone,
                         unsigned short val_iInst) { }
void CIteration::Solve(COutput *output,
                         CIntegration ****integration,
                         CGeometry ****geometry,
                         CSolver *****solver,
                         CNumerics ******numerics,
                         CConfig **config,
                         CSurfaceMovement **surface_movement,
                         CVolumetricMovement ***grid_movement,
                         CFreeFormDefBox*** FFDBox,
                         unsigned short val_iZone,
                         unsigned short val_iInst) { }
void CIteration::Update(COutput *output,
                        CIntegration ****integration,
                        CGeometry ****geometry,
                        CSolver *****solver,
                        CNumerics ******numerics,
                        CConfig **config,
                        CSurfaceMovement **surface_movement,
                        CVolumetricMovement ***grid_movement,
                        CFreeFormDefBox*** FFDBox,
                        unsigned short val_iZone,
                        unsigned short val_iInst)      { }
void CIteration::Predictor(COutput *output,
                        CIntegration ****integration,
                        CGeometry ****geometry,
                        CSolver *****solver,
                        CNumerics ******numerics,
                        CConfig **config,
                        CSurfaceMovement **surface_movement,
                        CVolumetricMovement ***grid_movement,
                        CFreeFormDefBox*** FFDBox,
                        unsigned short val_iZone,
                        unsigned short val_iInst)      { }
void CIteration::Relaxation(COutput *output,
                        CIntegration ****integration,
                        CGeometry ****geometry,
                        CSolver *****solver,
                        CNumerics ******numerics,
                        CConfig **config,
                        CSurfaceMovement **surface_movement,
                        CVolumetricMovement ***grid_movement,
                        CFreeFormDefBox*** FFDBox,
                        unsigned short val_iZone,
                        unsigned short val_iInst)      { }
bool CIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     { return false; }
void CIteration::Output(COutput *output,
    CGeometry ****geometry,
    CSolver *****solver,
    CConfig **config,
    unsigned long Iter,
    bool StopCalc,
    unsigned short val_iZone,
    unsigned short val_iInst)      {

  bool output_files = false;

  /*--- Determine whether a solution needs to be written
   after the current iteration ---*/

  if (

      /*--- General if statements to print output statements ---*/

//      (ExtIter+1 >= nExtIter) || (StopCalc) ||

      /*--- Fixed CL problem ---*/

      ((config[ZONE_0]->GetFixed_CL_Mode()) &&
       (config[ZONE_0]->GetnExtIter()-config[ZONE_0]->GetIter_dCL_dAlpha() - 1 == Iter)) ||

      /*--- Steady problems ---*/

      ((Iter % config[ZONE_0]->GetWrt_Sol_Freq() == 0) && (Iter != 0) &&
       ((config[ZONE_0]->GetUnsteady_Simulation() == STEADY) ||
        (config[ZONE_0]->GetUnsteady_Simulation() == HARMONIC_BALANCE) ||
        (config[ZONE_0]->GetUnsteady_Simulation() == ROTATIONAL_FRAME))) ||

      /*--- No inlet profile file found. Print template. ---*/

      (config[ZONE_0]->GetWrt_InletFile())

      ) {

    output_files = true;

  }

  /*--- Determine whether a solution doesn't need to be written
   after the current iteration ---*/

  if (config[ZONE_0]->GetFixed_CL_Mode()) {
    if (config[ZONE_0]->GetnExtIter()-config[ZONE_0]->GetIter_dCL_dAlpha() - 1 < Iter) output_files = false;
    if (config[ZONE_0]->GetnExtIter() - 1 == Iter) output_files = true;
  }

  /*--- write the solution ---*/

  if (output_files) {

    if (rank == MASTER_NODE) cout << endl << "-------------------------- File Output Summary --------------------------";

    /*--- Execute the routine for writing restart, volume solution,
     surface solution, and surface comma-separated value files. ---*/

    output->SetResult_Files_Parallel(solver, geometry, config, Iter, nZone);

    /*--- Execute the routine for writing special output. ---*/
    output->SetSpecial_Output(solver, geometry, config, Iter, nZone);


    if (rank == MASTER_NODE) cout << "-------------------------------------------------------------------------" << endl << endl;

  }

}
void CIteration::Postprocess(COutput *output,
                             CIntegration ****integration,
                             CGeometry ****geometry,
                             CSolver *****solver,
                             CNumerics ******numerics,
                             CConfig **config,
                             CSurfaceMovement **surface_movement,
                             CVolumetricMovement ***grid_movement,
                             CFreeFormDefBox*** FFDBox,
                             unsigned short val_iZone,
                             unsigned short val_iInst) { }



CFluidIteration::CFluidIteration(CConfig *config) : CIteration(config) { }
CFluidIteration::~CFluidIteration(void) { }

void CFluidIteration::Preprocess(COutput *output,
                                    CIntegration ****integration,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CNumerics ******numerics,
                                    CConfig **config,
                                    CSurfaceMovement **surface_movement,
                                    CVolumetricMovement ***grid_movement,
                                    CFreeFormDefBox*** FFDBox,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) {
  
  unsigned long IntIter = 0; config[val_iZone]->SetIntIter(IntIter);
  unsigned long ExtIter = config[val_iZone]->GetExtIter();
  
  bool fsi = config[val_iZone]->GetFSI_Simulation();
  unsigned long OuterIter = config[val_iZone]->GetOuterIter();

  
  /*--- Set the initial condition for FSI problems with subiterations ---*/
  /*--- This is done only in the first block subiteration.---*/
  /*--- From then on, the solver reuses the partially converged solution obtained in the previous subiteration ---*/
  if( fsi  && ( OuterIter == 0 ) ){
    solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->SetInitialCondition(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], ExtIter);
  }
  
  /*--- Apply a Wind Gust ---*/
  
  if (config[val_iZone]->GetWind_Gust()) {
    SetWind_GustField(config[val_iZone], geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst]);
  }

  /*--- Evaluate the new CFL number (adaptive). ---*/
  if ((config[val_iZone]->GetCFL_Adapt() == YES) && ( OuterIter != 0 ) ) {
    output->SetCFL_Number(solver, config, val_iZone);
  }

}

void CFluidIteration::Iterate(COutput *output,
                                 CIntegration ****integration,
                                 CGeometry ****geometry,
                                 CSolver *****solver,
                                 CNumerics ******numerics,
                                 CConfig **config,
                                 CSurfaceMovement **surface_movement,
                                 CVolumetricMovement ***grid_movement,
                                 CFreeFormDefBox*** FFDBox,
                                 unsigned short val_iZone,
                                 unsigned short val_iInst) {
  unsigned long IntIter, ExtIter;
  
  bool unsteady = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) || (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND);
  bool frozen_visc = (config[val_iZone]->GetContinuous_Adjoint() && config[val_iZone]->GetFrozen_Visc_Cont()) ||
                     (config[val_iZone]->GetDiscrete_Adjoint() && config[val_iZone]->GetFrozen_Visc_Disc());
  ExtIter = config[val_iZone]->GetExtIter();
  
  /* --- Setting up iteration values depending on if this is a
   steady or an unsteady simulaiton */
  
  if ( !unsteady ) IntIter = ExtIter;
  else IntIter = config[val_iZone]->GetIntIter();
  
  /*--- Update global parameters ---*/
  
  switch( config[val_iZone]->GetKind_Solver() ) {
      
    case EULER: case DISC_ADJ_EULER: case ONE_SHOT_EULER:
      config[val_iZone]->SetGlobalParam(EULER, RUNTIME_FLOW_SYS, ExtIter); break;
      
    case NAVIER_STOKES: case DISC_ADJ_NAVIER_STOKES: case ONE_SHOT_NAVIER_STOKES:
      config[val_iZone]->SetGlobalParam(NAVIER_STOKES, RUNTIME_FLOW_SYS, ExtIter); break;
      
    case RANS: case DISC_ADJ_RANS: case ONE_SHOT_RANS:
      config[val_iZone]->SetGlobalParam(RANS, RUNTIME_FLOW_SYS, ExtIter); break;
      
  }
  

  /*--- Solve the Euler, Navier-Stokes or Reynolds-averaged Navier-Stokes (RANS) equations (one iteration) ---*/
  
  integration[val_iZone][val_iInst][FLOW_SOL]->MultiGrid_Iteration(geometry, solver, numerics,
                                                                  config, RUNTIME_FLOW_SYS, IntIter, val_iZone, val_iInst);
  
  if ((config[val_iZone]->GetKind_Solver() == RANS) ||
      (((config[val_iZone]->GetKind_Solver() == DISC_ADJ_RANS) || (config[val_iZone]->GetKind_Solver() == ONE_SHOT_RANS)) && !frozen_visc)) {
    
    /*--- Solve the turbulence model ---*/
    
    config[val_iZone]->SetGlobalParam(RANS, RUNTIME_TURB_SYS, ExtIter);
    integration[val_iZone][val_iInst][TURB_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                     config, RUNTIME_TURB_SYS, IntIter, val_iZone, val_iInst);
    
    /*--- Solve transition model ---*/
    
    if (config[val_iZone]->GetKind_Trans_Model() == LM) {
      config[val_iZone]->SetGlobalParam(RANS, RUNTIME_TRANS_SYS, ExtIter);
      integration[val_iZone][val_iInst][TRANS_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                        config, RUNTIME_TRANS_SYS, IntIter, val_iZone, val_iInst);
    }
    
  }

  if (config[val_iZone]->GetWeakly_Coupled_Heat()){
    config[val_iZone]->SetGlobalParam(RANS, RUNTIME_HEAT_SYS, ExtIter);
    integration[val_iZone][val_iInst][HEAT_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                     config, RUNTIME_HEAT_SYS, IntIter, val_iZone, val_iInst);
  }
  
  /*--- Call Dynamic mesh update if AEROELASTIC motion was specified ---*/
  
  if ((config[val_iZone]->GetGrid_Movement()) && (config[val_iZone]->GetAeroelastic_Simulation()) && unsteady) {
      
    SetGrid_Movement(geometry[val_iZone][val_iInst], surface_movement[val_iZone], grid_movement[val_iZone][val_iInst],
                     solver[val_iZone][val_iInst], config[val_iZone], IntIter, ExtIter);
    
    /*--- Apply a Wind Gust ---*/
    
    if (config[val_iZone]->GetWind_Gust()) {
      if (IntIter % config[val_iZone]->GetAeroelasticIter() == 0 && IntIter != 0)
        SetWind_GustField(config[val_iZone], geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst]);
    }
    
  }
  
  
  /*--- Write the convergence history ---*/

  if ( unsteady && !config[val_iZone]->GetDiscrete_Adjoint() ) {
    
    output->SetConvHistory_Body(NULL, geometry, solver, config, integration, true, 0.0, val_iZone, val_iInst);
    
  }
  
}

void CFluidIteration::Update(COutput *output,
                                CIntegration ****integration,
                                CGeometry ****geometry,
                                CSolver *****solver,
                                CNumerics ******numerics,
                                CConfig **config,
                                CSurfaceMovement **surface_movement,
                                CVolumetricMovement ***grid_movement,
                                CFreeFormDefBox*** FFDBox,
                                unsigned short val_iZone,
                                unsigned short val_iInst)      {
  
  unsigned short iMesh;
  su2double Physical_dt, Physical_t;
  unsigned long ExtIter = config[val_iZone]->GetExtIter();

  /*--- Dual time stepping strategy ---*/
  
  if ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
      (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {
    
    /*--- Update dual time solver on all mesh levels ---*/
    
    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      integration[val_iZone][val_iInst][FLOW_SOL]->SetDualTime_Solver(geometry[val_iZone][val_iInst][iMesh], solver[val_iZone][val_iInst][iMesh][FLOW_SOL], config[val_iZone], iMesh);
      integration[val_iZone][val_iInst][FLOW_SOL]->SetConvergence(false);
    }
    
    /*--- Update dual time solver for the turbulence model ---*/
    
    if ((config[val_iZone]->GetKind_Solver() == RANS) ||
        (config[val_iZone]->GetKind_Solver() == ONE_SHOT_RANS) ||
        (config[val_iZone]->GetKind_Solver() == DISC_ADJ_RANS)) {
      integration[val_iZone][val_iInst][TURB_SOL]->SetDualTime_Solver(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0][TURB_SOL], config[val_iZone], MESH_0);
      integration[val_iZone][val_iInst][TURB_SOL]->SetConvergence(false);
    }
    
    /*--- Update dual time solver for the transition model ---*/
    
    if (config[val_iZone]->GetKind_Trans_Model() == LM) {
      integration[val_iZone][val_iInst][TRANS_SOL]->SetDualTime_Solver(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0][TRANS_SOL], config[val_iZone], MESH_0);
      integration[val_iZone][val_iInst][TRANS_SOL]->SetConvergence(false);
    }
    
    /*--- Verify convergence criteria (based on total time) ---*/
    
    Physical_dt = config[val_iZone]->GetDelta_UnstTime();
    Physical_t  = (ExtIter+1)*Physical_dt;
    if (Physical_t >=  config[val_iZone]->GetTotal_UnstTime())
      integration[val_iZone][val_iInst][FLOW_SOL]->SetConvergence(true);
    
  }
  
}

bool CFluidIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     {

  bool StopCalc = false;
  bool steady = (config[val_iZone]->GetUnsteady_Simulation() == STEADY);
  bool output_history = false;

#ifndef HAVE_MPI
  StopTime = su2double(clock())/su2double(CLOCKS_PER_SEC);
#else
  StopTime = MPI_Wtime();
#endif
  UsedTime = StopTime - StartTime;

  /*--- If convergence was reached --*/
  StopCalc = integration[val_iZone][INST_0][FLOW_SOL]->GetConvergence();

  /*--- Write the convergence history for the fluid (only screen output) ---*/

  /*--- The logic is right now case dependent ----*/
  /*--- This needs to be generalized when the new output structure comes ---*/
  output_history = (steady && !(multizone && (config[val_iZone]->GetnInner_Iter()==1)));

  if (output_history) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, false, UsedTime, val_iZone, INST_0);

  return StopCalc;


}
void CFluidIteration::Postprocess(COutput *output,
                                  CIntegration ****integration,
                                  CGeometry ****geometry,
                                  CSolver *****solver,
                                  CNumerics ******numerics,
                                  CConfig **config,
                                  CSurfaceMovement **surface_movement,
                                  CVolumetricMovement ***grid_movement,
                                  CFreeFormDefBox*** FFDBox,
                                  unsigned short val_iZone,
                                  unsigned short val_iInst) {

  /*--- Temporary: enable only for single-zone driver. This should be removed eventually when generalized. ---*/

  if(config[val_iZone]->GetSinglezone_Driver()){

    if (config[val_iZone]->GetKind_Solver() == DISC_ADJ_EULER ||
        config[val_iZone]->GetKind_Solver() == DISC_ADJ_NAVIER_STOKES ||
        config[val_iZone]->GetKind_Solver() == DISC_ADJ_RANS){

      /*--- Read the target pressure ---*/

      if (config[val_iZone]->GetInvDesign_Cp() == YES)
        output->SetCp_InverseDesign(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL],geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], config[val_iZone]->GetExtIter());

      /*--- Read the target heat flux ---*/

      if (config[val_iZone]->GetInvDesign_HeatFlux() == YES)
        output->SetHeatFlux_InverseDesign(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL],geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], config[val_iZone]->GetExtIter());

    }

  }


}

void CFluidIteration::Solve(COutput *output,
                                 CIntegration ****integration,
                                 CGeometry ****geometry,
                                 CSolver *****solver,
                                 CNumerics ******numerics,
                                 CConfig **config,
                                 CSurfaceMovement **surface_movement,
                                 CVolumetricMovement ***grid_movement,
                                 CFreeFormDefBox*** FFDBox,
                                 unsigned short val_iZone,
                                 unsigned short val_iInst) {

  /*--- Boolean to determine if we are running a static or dynamic case ---*/
  bool steady = (config[val_iZone]->GetUnsteady_Simulation() == STEADY);
  bool unsteady = ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) || (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND));

  unsigned short Inner_Iter, nInner_Iter = config[val_iZone]->GetnInner_Iter();
  bool StopCalc = false;

  /*--- Synchronization point before a single solver iteration. Compute the
   wall clock time required. ---*/

#ifndef HAVE_MPI
  StartTime = su2double(clock())/su2double(CLOCKS_PER_SEC);
#else
  StartTime = MPI_Wtime();
#endif

  /*--- If the problem is multizone, the block iterates on the number of internal iterations ---*/
  /*--- If the problem is single zone, the block iterates on the number of iterations (pseudo-time)---*/
  if (multizone)
    nInner_Iter = config[val_iZone]->GetnInner_Iter();
  else
    nInner_Iter = config[val_iZone]->GetnIter();

  /*--- Preprocess the solver ---*/
  Preprocess(output, integration, geometry,
      solver, numerics, config,
      surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

    /*--- For steady-state flow simulations, we need to loop over ExtIter for the number of time steps ---*/
    /*--- However, ExtIter is the number of FSI iterations, so nIntIter is used in this case ---*/

    for (Inner_Iter = 0; Inner_Iter < nInner_Iter; Inner_Iter++){

      /*--- For steady-state flow simulations, we need to loop over ExtIter for the number of time steps ---*/
      if (steady) config[val_iZone]->SetExtIter(Inner_Iter);
      /*--- For unsteady flow simulations, we need to loop over IntIter for the number of time steps ---*/
      if (unsteady) config[val_iZone]->SetIntIter(Inner_Iter);
      /*--- If only one internal iteration is required, the ExtIter/IntIter is the OuterIter of the block structure ---*/
      if (nInner_Iter == 1) {
        if (steady) config[val_iZone]->SetExtIter(config[val_iZone]->GetOuterIter());
        if (unsteady) config[val_iZone]->SetIntIter(config[val_iZone]->GetOuterIter());
      }

      /*--- Run a single iteration of the solver ---*/
      Iterate(output, integration, geometry,
          solver, numerics, config,
          surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

      /*--- Monitor the pseudo-time ---*/
      StopCalc = Monitor(output, integration, geometry,
                         solver, numerics, config,
                         surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

      /*--- Output files at intermediate time positions if the problem is single zone ---*/

      if (singlezone) Output(output, geometry, solver, config,
                             Inner_Iter, StopCalc, val_iZone, val_iInst);

      /*--- If the iteration has converged, break the loop ---*/
      if (StopCalc) break;

    }

    /*--- Set the fluid convergence to false (to make sure outer subiterations converge) ---*/
    if (multizone) integration[val_iZone][INST_0][FLOW_SOL]->SetConvergence(false);

}

void CFluidIteration::SetWind_GustField(CConfig *config, CGeometry **geometry, CSolver ***solver) {
  // The gust is imposed on the flow field via the grid velocities. This method called the Field Velocity Method is described in the
  // NASA TM–2012-217771 - Development, Verification and Use of Gust Modeling in the NASA Computational Fluid Dynamics Code FUN3D
  // the desired gust is prescribed as the negative of the grid velocity.
  
  // If a source term is included to account for the gust field, the method is described by Jones et al. as the Split Velocity Method in
  // Simulation of Airfoil Gust Responses Using Prescribed Velocities.
  // In this routine the gust derivatives needed for the source term are calculated when applicable.
  // If the gust derivatives are zero the source term is also zero.
  // The source term itself is implemented in the class CSourceWindGust
  
  if (rank == MASTER_NODE)
    cout << endl << "Running simulation with a Wind Gust." << endl;
  unsigned short iDim, nDim = geometry[MESH_0]->GetnDim(); //We assume nDim = 2
  if (nDim != 2) {
    if (rank == MASTER_NODE) {
      cout << endl << "WARNING - Wind Gust capability is only verified for 2 dimensional simulations." << endl;
    }
  }
  
  /*--- Gust Parameters from config ---*/
  unsigned short Gust_Type = config->GetGust_Type();
  su2double xbegin = config->GetGust_Begin_Loc();    // Location at which the gust begins.
  su2double L = config->GetGust_WaveLength();        // Gust size
  su2double tbegin = config->GetGust_Begin_Time();   // Physical time at which the gust begins.
  su2double gust_amp = config->GetGust_Ampl();       // Gust amplitude
  su2double n = config->GetGust_Periods();           // Number of gust periods
  unsigned short GustDir = config->GetGust_Dir(); // Gust direction
  
  /*--- Variables needed to compute the gust ---*/
  unsigned short Kind_Grid_Movement = config->GetKind_GridMovement();
  unsigned long iPoint;
  unsigned short iMGlevel, nMGlevel = config->GetnMGLevels();
  
  su2double x, y, x_gust, dgust_dx, dgust_dy, dgust_dt;
  su2double *Gust, *GridVel, *NewGridVel, *GustDer;
  
  su2double Physical_dt = config->GetDelta_UnstTime();
  unsigned long ExtIter = config->GetExtIter();
  su2double Physical_t = ExtIter*Physical_dt;
  
  su2double Uinf = solver[MESH_0][FLOW_SOL]->GetVelocity_Inf(0); // Assumption gust moves at infinity velocity
  
  Gust = new su2double [nDim];
  NewGridVel = new su2double [nDim];
  for (iDim = 0; iDim < nDim; iDim++) {
    Gust[iDim] = 0.0;
    NewGridVel[iDim] = 0.0;
  }
  
  GustDer = new su2double [3];
  for (unsigned short i = 0; i < 3; i++) {
    GustDer[i] = 0.0;
  }
  
  // Vortex variables
  unsigned long nVortex = 0;
  vector<su2double> x0, y0, vort_strenth, r_core; //vortex is positive in clockwise direction.
  if (Gust_Type == VORTEX) {
    InitializeVortexDistribution(nVortex, x0, y0, vort_strenth, r_core);
  }
  
  /*--- Check to make sure gust lenght is not zero or negative (vortex gust doesn't use this). ---*/
  if (L <= 0.0 && Gust_Type != VORTEX) {
    SU2_MPI::Error("The gust length needs to be positive", CURRENT_FUNCTION);
  }
  
  /*--- Loop over all multigrid levels ---*/
  
  for (iMGlevel = 0; iMGlevel <= nMGlevel; iMGlevel++) {
    
    /*--- Loop over each node in the volume mesh ---*/
    
    for (iPoint = 0; iPoint < geometry[iMGlevel]->GetnPoint(); iPoint++) {
      
      /*--- Reset the Grid Velocity to zero if there is no grid movement ---*/
      if (Kind_Grid_Movement == GUST) {
        for (iDim = 0; iDim < nDim; iDim++)
          geometry[iMGlevel]->node[iPoint]->SetGridVel(iDim, 0.0);
      }
      
      /*--- initialize the gust and derivatives to zero everywhere ---*/
      
      for (iDim = 0; iDim < nDim; iDim++) {Gust[iDim]=0.0;}
      dgust_dx = 0.0; dgust_dy = 0.0; dgust_dt = 0.0;
      
      /*--- Begin applying the gust ---*/
      
      if (Physical_t >= tbegin) {
        
        x = geometry[iMGlevel]->node[iPoint]->GetCoord()[0]; // x-location of the node.
        y = geometry[iMGlevel]->node[iPoint]->GetCoord()[1]; // y-location of the node.
        
        // Gust coordinate
        x_gust = (x - xbegin - Uinf*(Physical_t-tbegin))/L;
        
        /*--- Calculate the specified gust ---*/
        switch (Gust_Type) {
            
          case TOP_HAT:
            // Check if we are in the region where the gust is active
            if (x_gust > 0 && x_gust < n) {
              Gust[GustDir] = gust_amp;
              // Still need to put the gust derivatives. Think about this.
            }
            break;
            
          case SINE:
            // Check if we are in the region where the gust is active
            if (x_gust > 0 && x_gust < n) {
              Gust[GustDir] = gust_amp*(sin(2*PI_NUMBER*x_gust));
              
              // Gust derivatives
              //dgust_dx = gust_amp*2*PI_NUMBER*(cos(2*PI_NUMBER*x_gust))/L;
              //dgust_dy = 0;
              //dgust_dt = gust_amp*2*PI_NUMBER*(cos(2*PI_NUMBER*x_gust))*(-Uinf)/L;
            }
            break;
            
          case ONE_M_COSINE:
            // Check if we are in the region where the gust is active
            if (x_gust > 0 && x_gust < n) {
              Gust[GustDir] = gust_amp*(1-cos(2*PI_NUMBER*x_gust));
              
              // Gust derivatives
              //dgust_dx = gust_amp*2*PI_NUMBER*(sin(2*PI_NUMBER*x_gust))/L;
              //dgust_dy = 0;
              //dgust_dt = gust_amp*2*PI_NUMBER*(sin(2*PI_NUMBER*x_gust))*(-Uinf)/L;
            }
            break;
            
          case EOG:
            // Check if we are in the region where the gust is active
            if (x_gust > 0 && x_gust < n) {
              Gust[GustDir] = -0.37*gust_amp*sin(3*PI_NUMBER*x_gust)*(1-cos(2*PI_NUMBER*x_gust));
            }
            break;
            
          case VORTEX:
            
            /*--- Use vortex distribution ---*/
            // Algebraic vortex equation.
            for (unsigned long i=0; i<nVortex; i++) {
              su2double r2 = pow(x-(x0[i]+Uinf*(Physical_t-tbegin)), 2) + pow(y-y0[i], 2);
              su2double r = sqrt(r2);
              su2double v_theta = vort_strenth[i]/(2*PI_NUMBER) * r/(r2+pow(r_core[i],2));
              Gust[0] = Gust[0] + v_theta*(y-y0[i])/r;
              Gust[1] = Gust[1] - v_theta*(x-(x0[i]+Uinf*(Physical_t-tbegin)))/r;
            }
            break;
            
          case NONE: default:
            
            /*--- There is no wind gust specified. ---*/
            if (rank == MASTER_NODE) {
              cout << "No wind gust specified." << endl;
            }
            break;
            
        }
      }
      
      /*--- Set the Wind Gust, Wind Gust Derivatives and the Grid Velocities ---*/
      
      GustDer[0] = dgust_dx;
      GustDer[1] = dgust_dy;
      GustDer[2] = dgust_dt;
      
      solver[iMGlevel][FLOW_SOL]->node[iPoint]->SetWindGust(Gust);
      solver[iMGlevel][FLOW_SOL]->node[iPoint]->SetWindGustDer(GustDer);
      
      GridVel = geometry[iMGlevel]->node[iPoint]->GetGridVel();
      
      /*--- Store new grid velocity ---*/
      
      for (iDim = 0; iDim < nDim; iDim++) {
        NewGridVel[iDim] = GridVel[iDim] - Gust[iDim];
        geometry[iMGlevel]->node[iPoint]->SetGridVel(iDim, NewGridVel[iDim]);
      }
      
    }
  }
  
  delete [] Gust;
  delete [] GustDer;
  delete [] NewGridVel;
  
}

void CFluidIteration::InitializeVortexDistribution(unsigned long &nVortex, vector<su2double>& x0, vector<su2double>& y0, vector<su2double>& vort_strength, vector<su2double>& r_core) {
  /*--- Read in Vortex Distribution ---*/
  std::string line;
  std::ifstream file;
  su2double x_temp, y_temp, vort_strength_temp, r_core_temp;
  file.open("vortex_distribution.txt");
  /*--- In case there is no vortex file ---*/
  if (file.fail()) {
    SU2_MPI::Error("There is no vortex data file!!", CURRENT_FUNCTION);
  }
  
  // Ignore line containing the header
  getline(file, line);
  // Read in the information of the vortices (xloc, yloc, lambda(strength), eta(size, gradient))
  while (file.good())
  {
    getline(file, line);
    std::stringstream ss(line);
    if (line.size() != 0) { //ignore blank lines if they exist.
      ss >> x_temp;
      ss >> y_temp;
      ss >> vort_strength_temp;
      ss >> r_core_temp;
      x0.push_back(x_temp);
      y0.push_back(y_temp);
      vort_strength.push_back(vort_strength_temp);
      r_core.push_back(r_core_temp);
    }
  }
  file.close();
  // number of vortices
  nVortex = x0.size();
  
}


CTurboIteration::CTurboIteration(CConfig *config) : CFluidIteration(config) { }
CTurboIteration::~CTurboIteration(void) { }
void CTurboIteration::Preprocess(COutput *output,
                                    CIntegration ****integration,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CNumerics ******numerics,
                                    CConfig **config,
                                    CSurfaceMovement **surface_movement,
                                    CVolumetricMovement ***grid_movement,
                                    CFreeFormDefBox*** FFDBox,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) {

  /*--- Average quantities at the inflow and outflow boundaries ---*/ 
  solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->TurboAverageProcess(solver[val_iZone][val_iInst][MESH_0], geometry[val_iZone][val_iInst][MESH_0],config[val_iZone],INFLOW);
  solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->TurboAverageProcess(solver[val_iZone][val_iInst][MESH_0], geometry[val_iZone][val_iInst][MESH_0],config[val_iZone],OUTFLOW);

}

void CTurboIteration::Postprocess( COutput *output,
                                   CIntegration ****integration,
                                   CGeometry ****geometry,
                                   CSolver *****solver,
                                   CNumerics ******numerics,
                                   CConfig **config,
                                   CSurfaceMovement **surface_movement,
                                   CVolumetricMovement ***grid_movement,
                                   CFreeFormDefBox*** FFDBox,
                                   unsigned short val_iZone,
                                   unsigned short val_iInst) {

  /*--- Average quantities at the inflow and outflow boundaries ---*/
  solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->TurboAverageProcess(solver[val_iZone][val_iInst][MESH_0], geometry[val_iZone][val_iInst][MESH_0],config[val_iZone],INFLOW);
  solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->TurboAverageProcess(solver[val_iZone][val_iInst][MESH_0], geometry[val_iZone][val_iInst][MESH_0],config[val_iZone],OUTFLOW);
  
  /*--- Gather Inflow and Outflow quantities on the Master Node to compute performance ---*/
  solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GatherInOutAverageValues(config[val_iZone], geometry[val_iZone][val_iInst][MESH_0]);


  /*--- Compute turboperformance for single-zone adjoint cases. ---*/
  if (config[val_iZone]->GetSinglezone_Driver() && config[val_iZone]->GetDiscrete_Adjoint())
    output->ComputeTurboPerformance(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL], geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

}

CFEMFluidIteration::CFEMFluidIteration(CConfig *config) : CIteration(config) { }
CFEMFluidIteration::~CFEMFluidIteration(void) { }

void CFEMFluidIteration::Preprocess(COutput *output,
                                    CIntegration ****integration,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CNumerics ******numerics,
                                    CConfig **config,
                                    CSurfaceMovement **surface_movement,
                                    CVolumetricMovement ***grid_movement,
                                    CFreeFormDefBox*** FFDBox,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) {
  
  unsigned long IntIter = 0; config[ZONE_0]->SetIntIter(IntIter);
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();
  const bool restart = (config[ZONE_0]->GetRestart() ||
                        config[ZONE_0]->GetRestart_Flow());
  
  /*--- Set the initial condition if this is not a restart. ---*/
  if (ExtIter == 0 && !restart)
    solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->SetInitialCondition(geometry[val_iZone][val_iInst],
                                                                       solver[val_iZone][val_iInst],
                                                                       config[val_iZone],
                                                                       ExtIter);
  
}

void CFEMFluidIteration::Iterate(COutput *output,
                                 CIntegration ****integration,
                                 CGeometry ****geometry,
                                 CSolver *****solver,
                                 CNumerics ******numerics,
                                 CConfig **config,
                                 CSurfaceMovement **surface_movement,
                                 CVolumetricMovement ***grid_movement,
                                 CFreeFormDefBox*** FFDBox,
                                 unsigned short val_iZone,
                                 unsigned short val_iInst) {
  
  unsigned long IntIter = 0; config[ZONE_0]->SetIntIter(IntIter);
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();
  
  /*--- Update global parameters ---*/
  
  if (config[val_iZone]->GetKind_Solver() == FEM_EULER || config[val_iZone]->GetKind_Solver() == DISC_ADJ_FEM_EULER)
    config[val_iZone]->SetGlobalParam(FEM_EULER, RUNTIME_FLOW_SYS, ExtIter);
  
  if (config[val_iZone]->GetKind_Solver() == FEM_NAVIER_STOKES || config[val_iZone]->GetKind_Solver() == DISC_ADJ_FEM_NS)
    config[val_iZone]->SetGlobalParam(FEM_NAVIER_STOKES, RUNTIME_FLOW_SYS, ExtIter);
  
  if (config[val_iZone]->GetKind_Solver() == FEM_RANS || config[val_iZone]->GetKind_Solver() == DISC_ADJ_FEM_RANS)
    config[val_iZone]->SetGlobalParam(FEM_RANS, RUNTIME_FLOW_SYS, ExtIter);
  
  if (config[val_iZone]->GetKind_Solver() == FEM_LES)
    config[val_iZone]->SetGlobalParam(FEM_LES, RUNTIME_FLOW_SYS, ExtIter);
  
  /*--- Solve the Euler, Navier-Stokes, RANS or LES equations (one iteration) ---*/
  
  integration[val_iZone][val_iInst][FLOW_SOL]->SingleGrid_Iteration(geometry,
                                                                              solver,
                                                                              numerics,
                                                                              config,
                                                                              RUNTIME_FLOW_SYS,
                                                                              IntIter, val_iZone,
                                                                              val_iInst);
}

void CFEMFluidIteration::Update(COutput *output,
                                CIntegration ****integration,
                                CGeometry ****geometry,
                                CSolver *****solver,
                                CNumerics ******numerics,
                                CConfig **config,
                                CSurfaceMovement **surface_movement,
                                CVolumetricMovement ***grid_movement,
                                CFreeFormDefBox*** FFDBox,
                                unsigned short val_iZone,
                                unsigned short val_iInst)      { }
bool CFEMFluidIteration::Monitor(COutput *output,
                             CIntegration ****integration,
                             CGeometry ****geometry,
                             CSolver *****solver,
                             CNumerics ******numerics,
                             CConfig **config,
                             CSurfaceMovement **surface_movement,
                             CVolumetricMovement ***grid_movement,
                             CFreeFormDefBox*** FFDBox,
                             unsigned short val_iZone,
                             unsigned short val_iInst)     { return false; }
void CFEMFluidIteration::Postprocess(COutput *output,
                                 CIntegration ****integration,
                                 CGeometry ****geometry,
                                 CSolver *****solver,
                                 CNumerics ******numerics,
                                 CConfig **config,
                                 CSurfaceMovement **surface_movement,
                                 CVolumetricMovement ***grid_movement,
                                 CFreeFormDefBox*** FFDBox,
                                 unsigned short val_iZone,
                                 unsigned short val_iInst) { }

CHeatIteration::CHeatIteration(CConfig *config) : CIteration(config) { }

CHeatIteration::~CHeatIteration(void) { }

void CHeatIteration::Preprocess(COutput *output,
                                CIntegration ****integration,
                                CGeometry ****geometry,
                                CSolver *****solver,
                                CNumerics ******numerics,
                                CConfig **config,
                                CSurfaceMovement **surface_movement,
                                CVolumetricMovement ***grid_movement,
                                CFreeFormDefBox*** FFDBox,
                                unsigned short val_iZone,
                                unsigned short val_iInst) {

  unsigned long OuterIter = config[val_iZone]->GetOuterIter();

  /*--- Evaluate the new CFL number (adaptive). ---*/
  if ((config[val_iZone]->GetCFL_Adapt() == YES) && ( OuterIter != 0 ) ) {
    output->SetCFL_Number(solver, config, val_iZone);
  }

}

void CHeatIteration::Iterate(COutput *output,
                             CIntegration ****integration,
                             CGeometry ****geometry,
                             CSolver *****solver,
                             CNumerics ******numerics,
                             CConfig **config,
                             CSurfaceMovement **surface_movement,
                             CVolumetricMovement ***grid_movement,
                             CFreeFormDefBox*** FFDBox,
                             unsigned short val_iZone,
                             unsigned short val_iInst) {

  unsigned long IntIter, ExtIter;
  bool unsteady = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) || (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND);
  
  ExtIter = config[val_iZone]->GetExtIter();
  
  /* --- Setting up iteration values depending on if this is a
   steady or an unsteady simulaiton */

  if ( !unsteady ) IntIter = ExtIter;
  else IntIter = config[val_iZone]->GetIntIter();
  
  /*--- Update global parameters ---*/

  config[val_iZone]->SetGlobalParam(HEAT_EQUATION_FVM, RUNTIME_HEAT_SYS, ExtIter);

  integration[val_iZone][val_iInst][HEAT_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                   config, RUNTIME_HEAT_SYS, IntIter, val_iZone, val_iInst);
  
  /*--- Write the convergence history ---*/

  if ( unsteady && !config[val_iZone]->GetDiscrete_Adjoint() ) {

    output->SetConvHistory_Body(NULL, geometry, solver, config, integration, true, 0.0, val_iZone, val_iInst);
  }
}

void CHeatIteration::Update(COutput *output,
                            CIntegration ****integration,
                            CGeometry ****geometry,
                            CSolver *****solver,
                            CNumerics ******numerics,
                            CConfig **config,
                            CSurfaceMovement **surface_movement,
                            CVolumetricMovement ***grid_movement,
                            CFreeFormDefBox*** FFDBox,
                            unsigned short val_iZone,
                            unsigned short val_iInst)      {
  
  unsigned short iMesh;
  su2double Physical_dt, Physical_t;
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();
  
  /*--- Dual time stepping strategy ---*/
  if ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
      (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {
    
    /*--- Update dual time solver ---*/
    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      integration[val_iZone][val_iInst][HEAT_SOL]->SetDualTime_Solver(geometry[val_iZone][val_iInst][iMesh], solver[val_iZone][val_iInst][iMesh][HEAT_SOL], config[val_iZone], iMesh);
      integration[val_iZone][val_iInst][HEAT_SOL]->SetConvergence(false);
    }
    
    Physical_dt = config[val_iZone]->GetDelta_UnstTime();
    Physical_t  = (ExtIter+1)*Physical_dt;
    if (Physical_t >=  config[val_iZone]->GetTotal_UnstTime())
      integration[val_iZone][val_iInst][HEAT_SOL]->SetConvergence(true);
  }
}
bool CHeatIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     { return false; }
void CHeatIteration::Postprocess(COutput *output,
                                 CIntegration ****integration,
                                 CGeometry ****geometry,
                                 CSolver *****solver,
                                 CNumerics ******numerics,
                                 CConfig **config,
                                 CSurfaceMovement **surface_movement,
                                 CVolumetricMovement ***grid_movement,
                                 CFreeFormDefBox*** FFDBox,
                                 unsigned short val_iZone,
                                 unsigned short val_iInst) { }

void CHeatIteration::Solve(COutput *output,
                             CIntegration ****integration,
                             CGeometry ****geometry,
                             CSolver *****solver,
                             CNumerics ******numerics,
                             CConfig **config,
                             CSurfaceMovement **surface_movement,
                             CVolumetricMovement ***grid_movement,
                             CFreeFormDefBox*** FFDBox,
                             unsigned short val_iZone,
                             unsigned short val_iInst) {

  /*--- Boolean to determine if we are running a steady or unsteady case ---*/
  bool steady = (config[val_iZone]->GetUnsteady_Simulation() == STEADY);
  bool unsteady = ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) || (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND));

  unsigned short Inner_Iter, nInner_Iter = config[val_iZone]->GetnInner_Iter();
  bool StopCalc = false;

  /*--- Preprocess the solver ---*/
  Preprocess(output, integration, geometry,
      solver, numerics, config,
      surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

  /*--- For steady-state flow simulations, we need to loop over ExtIter for the number of time steps ---*/
  /*--- However, ExtIter is the number of FSI iterations, so nIntIter is used in this case ---*/

  for (Inner_Iter = 0; Inner_Iter < nInner_Iter; Inner_Iter++){

    /*--- For steady-state flow simulations, we need to loop over ExtIter for the number of time steps ---*/
    if (steady) config[val_iZone]->SetExtIter(Inner_Iter);
    /*--- For unsteady flow simulations, we need to loop over IntIter for the number of time steps ---*/
    if (unsteady) config[val_iZone]->SetIntIter(Inner_Iter);
    /*--- If only one internal iteration is required, the ExtIter/IntIter is the OuterIter of the block structure ---*/
    if (nInner_Iter == 1) {
      if (steady) config[val_iZone]->SetExtIter(config[val_iZone]->GetOuterIter());
      if (unsteady) config[val_iZone]->SetIntIter(config[val_iZone]->GetOuterIter());
    }

    Iterate(output, integration, geometry,
        solver, numerics, config,
        surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

    /*--- Write the convergence history for the fluid (only screen output) ---*/
    if (steady) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, false, 0.0, val_iZone, INST_0);

    /*--- If convergence was reached in every zone --*/
    StopCalc = integration[val_iZone][INST_0][HEAT_SOL]->GetConvergence();
    if (StopCalc) break;

  }

  /*--- Set the heat convergence to false (to make sure outer subiterations converge) ---*/
  integration[val_iZone][INST_0][HEAT_SOL]->SetConvergence(false);

  //output->SetConvHistory_Body(NULL, geometry, solver, config, integration, true, 0.0, val_iZone, INST_0);

}

CFEAIteration::CFEAIteration(CConfig *config) : CIteration(config) { }
CFEAIteration::~CFEAIteration(void) { }
void CFEAIteration::Preprocess() { }
void CFEAIteration::Iterate(COutput *output,
                                CIntegration ****integration,
                                CGeometry ****geometry,
                                CSolver *****solver,
                                CNumerics ******numerics,
                                CConfig **config,
                                CSurfaceMovement **surface_movement,
                                CVolumetricMovement ***grid_movement,
                                CFreeFormDefBox*** FFDBox,
                                unsigned short val_iZone,
                                unsigned short val_iInst
                                ) {

  su2double loadIncrement;
  unsigned long IntIter = 0; config[val_iZone]->SetIntIter(IntIter);
  unsigned long ExtIter = config[val_iZone]->GetExtIter();

  unsigned long iIncrement;
  unsigned long nIncrements = config[val_iZone]->GetNumberIncrements();

  bool nonlinear = (config[val_iZone]->GetGeometricConditions() == LARGE_DEFORMATIONS);  // Geometrically non-linear problems
  bool linear = (config[val_iZone]->GetGeometricConditions() == SMALL_DEFORMATIONS);  // Geometrically non-linear problems

  bool disc_adj_fem = false;
  if (config[val_iZone]->GetKind_Solver() == DISC_ADJ_FEM) disc_adj_fem = true;

  bool write_output = true;

  bool incremental_load = config[val_iZone]->GetIncrementalLoad();              // If an incremental load is applied

  ofstream ConvHist_file;

  /*--- This is to prevent problems when running a linear solver ---*/
  if (!nonlinear) incremental_load = false;

  /*--- Set the convergence monitor to false, to prevent the solver to stop in intermediate FSI subiterations ---*/
  integration[val_iZone][val_iInst][FEA_SOL]->SetConvergence(false);

  if (linear) {

    /*--- Set the value of the internal iteration ---*/

    IntIter = ExtIter;

    /*--- FEA equations ---*/

    config[val_iZone]->SetGlobalParam(FEM_ELASTICITY, RUNTIME_FEA_SYS, ExtIter);

    /*--- Run the iteration ---*/

    integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
        config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);

  }
  /*--- If the structure is held static and the solver is nonlinear, we don't need to solve for static time, but we need to compute Mass Matrix and Integration constants ---*/
  else if (nonlinear) {

    /*--- THIS IS THE DIRECT APPROACH (NO INCREMENTAL LOAD APPLIED) ---*/

    if (!incremental_load) {

      /*--- Set the value of the internal iteration ---*/

      IntIter = 0;

      /*--- FEA equations ---*/

      config[val_iZone]->SetGlobalParam(FEM_ELASTICITY, RUNTIME_FEA_SYS, ExtIter);

      /*--- Write the convergence history headers ---*/

      if (!disc_adj_fem) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, true, 0.0, val_iZone, val_iInst);

      /*--- Run the iteration ---*/

      integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
          config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);


      /*----------------- If the solver is non-linear, we need to subiterate using a Newton-Raphson approach ----------------------*/

      for (IntIter = 1; IntIter < config[val_iZone]->GetDyn_nIntIter(); IntIter++) {

        /*--- Limits to only one structural iteration for the discrete adjoint FEM problem ---*/
        if (disc_adj_fem) break;

        /*--- Write the convergence history (first, compute Von Mises stress) ---*/
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
        write_output = output->PrintOutput(IntIter-1, config[val_iZone]->GetWrt_Con_Freq_DualTime());
        if (write_output) output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);

        config[val_iZone]->SetIntIter(IntIter);

        integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
            config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);

        if (integration[val_iZone][val_iInst][FEA_SOL]->GetConvergence()) break;

      }

    }
    /*--- The incremental load is only used in nonlinear cases ---*/
    else if (incremental_load) {

      /*--- Set the initial condition: store the current solution as Solution_Old ---*/

      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->SetInitialCondition(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], ExtIter);

      /*--- The load increment is 1.0 ---*/
      loadIncrement = 1.0;
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->SetLoad_Increment(loadIncrement);
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->SetForceCoeff(loadIncrement);

      /*--- Set the value of the internal iteration ---*/

      IntIter = 0;

      /*--- FEA equations ---*/

      config[val_iZone]->SetGlobalParam(FEM_ELASTICITY, RUNTIME_FEA_SYS, ExtIter);

      /*--- Write the convergence history headers ---*/

      if (!disc_adj_fem) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);

      /*--- Run the first iteration ---*/

      integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
          config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);


      /*--- Write the convergence history (first, compute Von Mises stress) ---*/
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
      output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);

      /*--- Run the second iteration ---*/

      IntIter = 1;

      config[val_iZone]->SetIntIter(IntIter);

      integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
          config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);

      /*--- Write the convergence history (first, compute Von Mises stress) ---*/
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
      output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);


      bool meetCriteria;
      su2double Residual_UTOL, Residual_RTOL, Residual_ETOL;
      su2double Criteria_UTOL, Criteria_RTOL, Criteria_ETOL;

      Criteria_UTOL = config[val_iZone]->GetIncLoad_Criteria(0);
      Criteria_RTOL = config[val_iZone]->GetIncLoad_Criteria(1);
      Criteria_ETOL = config[val_iZone]->GetIncLoad_Criteria(2);

      Residual_UTOL = log10(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetRes_FEM(0));
      Residual_RTOL = log10(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetRes_FEM(1));
      Residual_ETOL = log10(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetRes_FEM(2));

      meetCriteria = ( ( Residual_UTOL <  Criteria_UTOL ) &&
          ( Residual_RTOL <  Criteria_RTOL ) &&
          ( Residual_ETOL <  Criteria_ETOL ) );

      /*--- If the criteria is met and the load is not "too big", do the regular calculation ---*/
      if (meetCriteria) {

        for (IntIter = 2; IntIter < config[val_iZone]->GetDyn_nIntIter(); IntIter++) {

          /*--- Write the convergence history (first, compute Von Mises stress) ---*/
          solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
          output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);

          config[val_iZone]->SetIntIter(IntIter);

          integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
              config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);

          if (integration[val_iZone][val_iInst][FEA_SOL]->GetConvergence()) break;

        }

      }

      /*--- If the criteria is not met, a whole set of subiterations for the different loads must be done ---*/

      else {

        /*--- Here we have to restart the solution to the original one of the iteration ---*/
        /*--- Retrieve the Solution_Old as the current solution before subiterating ---*/

        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->ResetInitialCondition(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], ExtIter);

        /*--- For the number of increments ---*/
        for (iIncrement = 0; iIncrement < nIncrements; iIncrement++) {

          loadIncrement = (iIncrement + 1.0) * (1.0 / nIncrements);

          /*--- Set the load increment and the initial condition, and output the parameters of UTOL, RTOL, ETOL for the previous iteration ---*/

          /*--- Set the convergence monitor to false, to force se solver to converge every subiteration ---*/
          integration[val_iZone][val_iInst][FEA_SOL]->SetConvergence(false);


          /*--- FEA equations ---*/

          config[val_iZone]->SetGlobalParam(FEM_ELASTICITY, RUNTIME_FEA_SYS, ExtIter);


          solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->SetLoad_Increment(loadIncrement);

          if (rank == MASTER_NODE) {
            cout << endl;
            cout << "-- Incremental load: increment " << iIncrement + 1 << " ----------------------------------------" << endl;
          }

          /*--- Set the value of the internal iteration ---*/
          IntIter = 0;
          config[val_iZone]->SetIntIter(IntIter);

          /*--- FEA equations ---*/

          config[val_iZone]->SetGlobalParam(FEM_ELASTICITY, RUNTIME_FEA_SYS, ExtIter);

          /*--- Run the iteration ---*/

          integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
              config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);


          /*----------------- If the solver is non-linear, we need to subiterate using a Newton-Raphson approach ----------------------*/

          for (IntIter = 1; IntIter < config[val_iZone]->GetDyn_nIntIter(); IntIter++) {

            /*--- Write the convergence history (first, compute Von Mises stress) ---*/
            solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
            output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);

            config[val_iZone]->SetIntIter(IntIter);

            integration[val_iZone][val_iInst][FEA_SOL]->Structural_Iteration(geometry, solver, numerics,
                config, RUNTIME_FEA_SYS, IntIter, val_iZone, val_iInst);

            if (integration[val_iZone][val_iInst][FEA_SOL]->GetConvergence()) break;

          }

          /*--- Write history for intermediate steps ---*/
          if (iIncrement < nIncrements - 1){
            /*--- Write the convergence history (first, compute Von Mises stress) ---*/
            solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
            output->SetConvHistory_Body(&ConvHist_file, geometry, solver, config, integration, false, 0.0, val_iZone, val_iInst);
          }

        }

      }

    }


  }


  /*--- Finally, we need to compute the objective function, in case that we are running a discrete adjoint solver... ---*/

  switch (config[val_iZone]->GetKind_ObjFunc()){
    case REFERENCE_GEOMETRY:
      if ((config[val_iZone]->GetDV_FEA() == YOUNG_MODULUS) || (config[val_iZone]->GetDV_FEA() == DENSITY_VAL)){
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Stiffness_Penalty(geometry[val_iZone][val_iInst][MESH_0],solver[val_iZone][val_iInst][MESH_0],
          numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
      }
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_OFRefGeom(geometry[val_iZone][val_iInst][MESH_0],solver[val_iZone][val_iInst][MESH_0], config[val_iZone]);
      break;
    case REFERENCE_NODE:
      if ((config[val_iZone]->GetDV_FEA() == YOUNG_MODULUS) || (config[val_iZone]->GetDV_FEA() == DENSITY_VAL)){
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Stiffness_Penalty(geometry[val_iZone][val_iInst][MESH_0],solver[val_iZone][val_iInst][MESH_0],
          numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);
      }
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_OFRefNode(geometry[val_iZone][val_iInst][MESH_0],solver[val_iZone][val_iInst][MESH_0], config[val_iZone]);
      break;
    case VOLUME_FRACTION:
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_OFVolFrac(geometry[val_iZone][val_iInst][MESH_0],solver[val_iZone][val_iInst][MESH_0], config[val_iZone]);
      break;
  }

}

void CFEAIteration::Update(COutput *output,
       CIntegration ****integration,
       CGeometry ****geometry,
       CSolver *****solver,
       CNumerics ******numerics,
       CConfig **config,
       CSurfaceMovement **surface_movement,
       CVolumetricMovement ***grid_movement,
       CFreeFormDefBox*** FFDBox,
       unsigned short val_iZone,
       unsigned short val_iInst) {

  su2double Physical_dt, Physical_t;
    unsigned long ExtIter = config[val_iZone]->GetExtIter();
  bool dynamic = (config[val_iZone]->GetDynamic_Analysis() == DYNAMIC);          // Dynamic problems
  bool static_fem = (config[val_iZone]->GetDynamic_Analysis() == STATIC);         // Static problems
  bool fsi = config[val_iZone]->GetFSI_Simulation();         // Fluid-Structure Interaction problems


  /*----------------- Compute averaged nodal stress and reactions ------------------------*/

  solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->Compute_NodalStress(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL], config[val_iZone]);

  /*----------------- Update structural solver ----------------------*/

  if (dynamic) {
    integration[val_iZone][val_iInst][FEA_SOL]->SetFEM_StructuralSolver(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], config[val_iZone], MESH_0);
    integration[val_iZone][val_iInst][FEA_SOL]->SetConvergence(false);

      /*--- Verify convergence criteria (based on total time) ---*/

    Physical_dt = config[val_iZone]->GetDelta_DynTime();
    Physical_t  = (ExtIter+1)*Physical_dt;
    if (Physical_t >=  config[val_iZone]->GetTotal_DynTime())
      integration[val_iZone][val_iInst][FEA_SOL]->SetConvergence(true);
    } else if ( static_fem && fsi) {

    /*--- For FSI problems, output the relaxed result, which is the one transferred into the fluid domain (for restart purposes) ---*/
    switch (config[val_iZone]->GetKind_TimeIntScheme_FEA()) {
    case (NEWMARK_IMPLICIT):
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->ImplicitNewmark_Relaxation(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0], config[val_iZone]);
    break;

    }
  }

}

void CFEAIteration::Predictor(COutput *output,
                        CIntegration ****integration,
                        CGeometry ****geometry,
                        CSolver *****solver,
                        CNumerics ******numerics,
                        CConfig **config,
                        CSurfaceMovement **surface_movement,
                        CVolumetricMovement ***grid_movement,
                        CFreeFormDefBox*** FFDBox,
                        unsigned short val_iZone,
                        unsigned short val_iInst)      {

  /*--- Predict displacements ---*/

  solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->PredictStruct_Displacement(geometry[val_iZone][val_iInst], config[val_iZone],
      solver[val_iZone][val_iInst]);

  /*--- For parallel simulations we need to communicate the predicted solution before updating the fluid mesh ---*/
  
  solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->InitiateComms(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], SOLUTION_PRED);
  solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->CompleteComms(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], SOLUTION_PRED);

}
void CFEAIteration::Relaxation(COutput *output,
                        CIntegration ****integration,
                        CGeometry ****geometry,
                        CSolver *****solver,
                        CNumerics ******numerics,
                        CConfig **config,
                        CSurfaceMovement **surface_movement,
                        CVolumetricMovement ***grid_movement,
                        CFreeFormDefBox*** FFDBox,
                        unsigned short val_iZone,
                        unsigned short val_iInst)      {

  unsigned long OuterIter = config[val_iZone]->GetOuterIter();

  /*-------------------- Aitken's relaxation ------------------------*/

  /*------------------- Compute the coefficient ---------------------*/

  solver[val_iZone][INST_0][MESH_0][FEA_SOL]->ComputeAitken_Coefficient(geometry[val_iZone][INST_0], config[val_iZone],
      solver[val_iZone][INST_0], OuterIter);

  /*----------------- Set the relaxation parameter ------------------*/

  solver[val_iZone][INST_0][MESH_0][FEA_SOL]->SetAitken_Relaxation(geometry[val_iZone][INST_0], config[val_iZone],
      solver[val_iZone][INST_0]);

  /*----------------- Communicate the predicted solution and the old one ------------------*/

  solver[val_iZone][INST_0][MESH_0][FEA_SOL]->InitiateComms(geometry[val_iZone][INST_0][MESH_0], config[val_iZone], SOLUTION_PRED_OLD);
  solver[val_iZone][INST_0][MESH_0][FEA_SOL]->CompleteComms(geometry[val_iZone][INST_0][MESH_0], config[val_iZone], SOLUTION_PRED_OLD);

}

bool CFEAIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     { return false; }
void CFEAIteration::Postprocess(COutput *output,
                                          CIntegration ****integration,
                                          CGeometry ****geometry,
                                          CSolver *****solver,
                                          CNumerics ******numerics,
                                          CConfig **config,
                                          CSurfaceMovement **surface_movement,
                                          CVolumetricMovement ***grid_movement,
                                          CFreeFormDefBox*** FFDBox,
                                          unsigned short val_iZone,
                                          unsigned short val_iInst) { }

void CFEAIteration::Solve(COutput *output,
                                CIntegration ****integration,
                                CGeometry ****geometry,
                                CSolver *****solver,
                                CNumerics ******numerics,
                                CConfig **config,
                                CSurfaceMovement **surface_movement,
                                CVolumetricMovement ***grid_movement,
                                CFreeFormDefBox*** FFDBox,
                                unsigned short val_iZone,
                                unsigned short val_iInst
                                ) {

  bool multizone = config[val_iZone]->GetMultizone_Problem();

  /*------------------ Structural subiteration ----------------------*/
  Iterate(output, integration, geometry,
      solver, numerics, config,
      surface_movement, grid_movement, FFDBox, val_iZone, INST_0);

  /*--- Write the convergence history for the structure (only screen output) ---*/
  if (multizone) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, false, 0.0, val_iZone, INST_0);

  /*--- Set the structural convergence to false (to make sure outer subiterations converge) ---*/
  integration[val_iZone][INST_0][FEA_SOL]->SetConvergence(false);

}

CAdjFluidIteration::CAdjFluidIteration(CConfig *config) : CIteration(config) { }
CAdjFluidIteration::~CAdjFluidIteration(void) { }
void CAdjFluidIteration::Preprocess(COutput *output,
                                       CIntegration ****integration,
                                       CGeometry ****geometry,
                                       CSolver *****solver,
                                       CNumerics ******numerics,
                                       CConfig **config,
                                       CSurfaceMovement **surface_movement,
                                       CVolumetricMovement ***grid_movement,
                                       CFreeFormDefBox*** FFDBox,
                                       unsigned short val_iZone,
                                       unsigned short val_iInst) {
  
  unsigned short iMesh;
  bool harmonic_balance = (config[ZONE_0]->GetUnsteady_Simulation() == HARMONIC_BALANCE);
  bool dynamic_mesh = config[ZONE_0]->GetGrid_Movement();
  unsigned long IntIter = 0; config[ZONE_0]->SetIntIter(IntIter);
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();

  /*--- For the unsteady adjoint, load a new direct solution from a restart file. ---*/
  
  if (((dynamic_mesh && ExtIter == 0) || config[val_iZone]->GetUnsteady_Simulation()) && !harmonic_balance) {
    int Direct_Iter = SU2_TYPE::Int(config[val_iZone]->GetUnst_AdjointIter()) - SU2_TYPE::Int(ExtIter) - 1;
    if (rank == MASTER_NODE && val_iZone == ZONE_0 && config[val_iZone]->GetUnsteady_Simulation())
      cout << endl << " Loading flow solution from direct iteration " << Direct_Iter << "." << endl;
    solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->LoadRestart(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], Direct_Iter, true);
  }
  
  /*--- Continuous adjoint Euler, Navier-Stokes or Reynolds-averaged Navier-Stokes (RANS) equations ---*/
  
  if ((ExtIter == 0) || config[val_iZone]->GetUnsteady_Simulation()) {
    
    if (config[val_iZone]->GetKind_Solver() == ADJ_EULER)
      config[val_iZone]->SetGlobalParam(ADJ_EULER, RUNTIME_FLOW_SYS, ExtIter);
    if (config[val_iZone]->GetKind_Solver() == ADJ_NAVIER_STOKES)
      config[val_iZone]->SetGlobalParam(ADJ_NAVIER_STOKES, RUNTIME_FLOW_SYS, ExtIter);
    if (config[val_iZone]->GetKind_Solver() == ADJ_RANS)
      config[val_iZone]->SetGlobalParam(ADJ_RANS, RUNTIME_FLOW_SYS, ExtIter);
    
    /*--- Solve the Euler, Navier-Stokes or Reynolds-averaged Navier-Stokes (RANS) equations (one iteration) ---*/
    
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << "Begin direct solver to store flow data (single iteration)." << endl;
    
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << "Compute residuals to check the convergence of the direct problem." << endl;
    
    integration[val_iZone][val_iInst][FLOW_SOL]->MultiGrid_Iteration(geometry, solver, numerics,
                                                                    config, RUNTIME_FLOW_SYS, 0, val_iZone, val_iInst);
    
    if (config[val_iZone]->GetKind_Solver() == ADJ_RANS) {
      
      /*--- Solve the turbulence model ---*/
      
      config[val_iZone]->SetGlobalParam(ADJ_RANS, RUNTIME_TURB_SYS, ExtIter);
      integration[val_iZone][val_iInst][TURB_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                       config, RUNTIME_TURB_SYS, IntIter, val_iZone, val_iInst);
      
      /*--- Solve transition model ---*/
      
      if (config[val_iZone]->GetKind_Trans_Model() == LM) {
        config[val_iZone]->SetGlobalParam(RANS, RUNTIME_TRANS_SYS, ExtIter);
        integration[val_iZone][val_iInst][TRANS_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                          config, RUNTIME_TRANS_SYS, IntIter, val_iZone, val_iInst);
      }
      
    }
    
    /*--- Output the residual (visualization purpouses to identify if
     the direct solution is converged)---*/
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << "log10[Maximum residual]: " << log10(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetRes_Max(0))
      <<", located at point "<< solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetPoint_Max(0) << "." << endl;
    
    /*--- Compute gradients of the flow variables, this is necessary for sensitivity computation,
     note that in the direct Euler problem we are not computing the gradients of the primitive variables ---*/
    
    if (config[val_iZone]->GetKind_Gradient_Method() == GREEN_GAUSS)
      solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->SetPrimitive_Gradient_GG(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);
    if (config[val_iZone]->GetKind_Gradient_Method() == WEIGHTED_LEAST_SQUARES)
      solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->SetPrimitive_Gradient_LS(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);
    
    /*--- Set contribution from cost function for boundary conditions ---*/
    
    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      
      /*--- Set the value of the non-dimensional coefficients in the coarse levels, using the fine level solution ---*/
      
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->SetTotal_CD(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetTotal_CD());
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->SetTotal_CL(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetTotal_CL());
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->SetTotal_CT(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetTotal_CT());
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->SetTotal_CQ(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->GetTotal_CQ());
      
      /*--- Compute the adjoint boundary condition on Euler walls ---*/
      
      solver[val_iZone][val_iInst][iMesh][ADJFLOW_SOL]->SetForceProj_Vector(geometry[val_iZone][val_iInst][iMesh], solver[val_iZone][val_iInst][iMesh], config[val_iZone]);
      
      /*--- Set the internal boundary condition on nearfield surfaces ---*/
      
      if ((config[val_iZone]->GetKind_ObjFunc() == EQUIVALENT_AREA) ||
          (config[val_iZone]->GetKind_ObjFunc() == NEARFIELD_PRESSURE))
        solver[val_iZone][val_iInst][iMesh][ADJFLOW_SOL]->SetIntBoundary_Jump(geometry[val_iZone][val_iInst][iMesh], solver[val_iZone][val_iInst][iMesh], config[val_iZone]);
      
    }
    
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << "End direct solver, begin adjoint problem." << endl;
    
  }
  
}
void CAdjFluidIteration::Iterate(COutput *output,
                                    CIntegration ****integration,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CNumerics ******numerics,
                                    CConfig **config,
                                    CSurfaceMovement **surface_movement,
                                    CVolumetricMovement ***grid_movement,
                                    CFreeFormDefBox*** FFDBox,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) {
  
  unsigned long IntIter = 0; config[ZONE_0]->SetIntIter(IntIter);
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();
  bool unsteady = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) || (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND);
  
  /*--- Set the value of the internal iteration ---*/
  
  ExtIter = config[val_iZone]->GetExtIter();
  
  /* --- Setting up iteration values depending on if this is a 
  steady or an unsteady simulaiton */

  if ( !unsteady ) 
    IntIter = ExtIter;
  else
    IntIter = config[val_iZone]->GetIntIter();
    
    
  switch( config[val_iZone]->GetKind_Solver() ) {

  case ADJ_EULER:
    config[val_iZone]->SetGlobalParam(ADJ_EULER, RUNTIME_ADJFLOW_SYS, ExtIter); break;

  case ADJ_NAVIER_STOKES:
    config[val_iZone]->SetGlobalParam(ADJ_NAVIER_STOKES, RUNTIME_ADJFLOW_SYS, ExtIter); break;

  case ADJ_RANS:
    config[val_iZone]->SetGlobalParam(ADJ_RANS, RUNTIME_ADJFLOW_SYS, ExtIter); break;          
  }
    
  /*--- Iteration of the flow adjoint problem ---*/
  
  integration[val_iZone][val_iInst][ADJFLOW_SOL]->MultiGrid_Iteration(geometry, solver, numerics,
                                                                     config, RUNTIME_ADJFLOW_SYS, IntIter, val_iZone, val_iInst);
  
  /*--- Iteration of the turbulence model adjoint ---*/
  
  if ((config[val_iZone]->GetKind_Solver() == ADJ_RANS) && (!config[val_iZone]->GetFrozen_Visc_Cont())) {
    
    /*--- Adjoint turbulence model solution ---*/
    
    config[val_iZone]->SetGlobalParam(ADJ_RANS, RUNTIME_ADJTURB_SYS, ExtIter);
    integration[val_iZone][val_iInst][ADJTURB_SOL]->SingleGrid_Iteration(geometry, solver, numerics,
                                                                        config, RUNTIME_ADJTURB_SYS, IntIter, val_iZone, val_iInst);
    
  }
  
}
void CAdjFluidIteration::Update(COutput *output,
                                   CIntegration ****integration,
                                   CGeometry ****geometry,
                                   CSolver *****solver,
                                   CNumerics ******numerics,
                                   CConfig **config,
                                   CSurfaceMovement **surface_movement,
                                   CVolumetricMovement ***grid_movement,
                                   CFreeFormDefBox*** FFDBox,
                                   unsigned short val_iZone,
                                   unsigned short val_iInst)      {
  
  su2double Physical_dt, Physical_t;
  unsigned short iMesh;
  unsigned long ExtIter = config[ZONE_0]->GetExtIter();
  
  /*--- Dual time stepping strategy ---*/
  
  if ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
      (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {
    
    /*--- Update dual time solver ---*/
    
    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      integration[val_iZone][val_iInst][ADJFLOW_SOL]->SetDualTime_Solver(geometry[val_iZone][val_iInst][iMesh], solver[val_iZone][val_iInst][iMesh][ADJFLOW_SOL], config[val_iZone], iMesh);
      integration[val_iZone][val_iInst][ADJFLOW_SOL]->SetConvergence(false);
    }
    
    Physical_dt = config[val_iZone]->GetDelta_UnstTime(); Physical_t  = (ExtIter+1)*Physical_dt;
    if (Physical_t >=  config[val_iZone]->GetTotal_UnstTime()) integration[val_iZone][val_iInst][ADJFLOW_SOL]->SetConvergence(true);
    
  }
}

bool CAdjFluidIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     { return false; }
void CAdjFluidIteration::Postprocess(COutput *output,
                                     CIntegration ****integration,
                                     CGeometry ****geometry,
                                     CSolver *****solver,
                                     CNumerics ******numerics,
                                     CConfig **config,
                                     CSurfaceMovement **surface_movement,
                                     CVolumetricMovement ***grid_movement,
                                     CFreeFormDefBox*** FFDBox,
                                     unsigned short val_iZone,
                                     unsigned short val_iInst) { }

CDiscAdjFluidIteration::CDiscAdjFluidIteration(CConfig *config) : CIteration(config) {
  
  turbulent = ( config->GetKind_Solver() == DISC_ADJ_RANS);
  
}

CDiscAdjFluidIteration::~CDiscAdjFluidIteration(void) { }

void CDiscAdjFluidIteration::Preprocess(COutput *output,
                                           CIntegration ****integration,
                                           CGeometry ****geometry,
                                           CSolver *****solver,
                                           CNumerics ******numerics,
                                           CConfig **config,
                                           CSurfaceMovement **surface_movement,
                                           CVolumetricMovement ***grid_movement,
                                           CFreeFormDefBox*** FFDBox,
                                           unsigned short val_iZone,
                                           unsigned short val_iInst) {

#ifndef HAVE_MPI
  StartTime = su2double(clock())/su2double(CLOCKS_PER_SEC);
#else
  StartTime = MPI_Wtime();
#endif

  unsigned long IntIter = 0, iPoint;
  config[ZONE_0]->SetIntIter(IntIter);
  unsigned short ExtIter = config[val_iZone]->GetExtIter();
  bool dual_time_1st = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST);
  bool dual_time_2nd = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND);
  bool dual_time = (dual_time_1st || dual_time_2nd);
  unsigned short iMesh;
  int Direct_Iter;
  bool heat = config[val_iZone]->GetWeakly_Coupled_Heat();

  /*--- Read the target pressure for inverse design. ---------------------------------------------*/
  if (config[val_iZone]->GetInvDesign_Cp() == YES)
    output->SetCp_InverseDesign(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL], geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], ExtIter);

  /*--- Read the target heat flux ----------------------------------------------------------------*/
  if (config[ZONE_0]->GetInvDesign_HeatFlux() == YES)
    output->SetHeatFlux_InverseDesign(solver[val_iZone][val_iInst][MESH_0][FLOW_SOL], geometry[val_iZone][val_iInst][MESH_0], config[val_iZone], ExtIter);

  /*--- For the unsteady adjoint, load direct solutions from restart files. ---*/

  if (config[val_iZone]->GetUnsteady_Simulation()) {

    Direct_Iter = SU2_TYPE::Int(config[val_iZone]->GetUnst_AdjointIter()) - SU2_TYPE::Int(ExtIter) - 2;

    /*--- For dual-time stepping we want to load the already converged solution at timestep n ---*/

    if (dual_time) {
      Direct_Iter += 1;
    }

    if (ExtIter == 0){

      if (dual_time_2nd) {

        /*--- Load solution at timestep n-2 ---*/

        LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter-2);

        /*--- Push solution back to correct array ---*/

        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n();
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n1();
            if (turbulent) {
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n();
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n1();
            }
            if (heat) {
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n();
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n1();
            }
          }
        }
      }
      if (dual_time) {

        /*--- Load solution at timestep n-1 ---*/

        LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter-1);

        /*--- Push solution back to correct array ---*/

        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n();
            if (turbulent) {
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n();
            }
            if (heat) {
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n();
            }
          }
        }
      }

      /*--- Load solution timestep n ---*/

      LoadUnsteady_Solution(geometry, solver,config, val_iInst, val_iZone, Direct_Iter);

    }


    if ((ExtIter > 0) && dual_time){

      /*--- Load solution timestep n-1 | n-2 for DualTimestepping 1st | 2nd order ---*/
      if (dual_time_1st){
        LoadUnsteady_Solution(geometry, solver,config, val_iInst, val_iZone, Direct_Iter - 1);
      } else {
        LoadUnsteady_Solution(geometry, solver,config, val_iInst, val_iZone, Direct_Iter - 2);
      }
  

      /*--- Temporarily store the loaded solution in the Solution_Old array ---*/

      for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
        for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
           solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_OldSolution();
           if (turbulent){
             solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_OldSolution();
           }
           if (heat){
             solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_OldSolution();
           }
        }
      }

      /*--- Set Solution at timestep n to solution at n-1 ---*/

      for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
        for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
          solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->SetSolution(solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->GetSolution_time_n());
          if (turbulent) {
            solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->SetSolution(solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->GetSolution_time_n());
          }
          if (heat) {
            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->SetSolution(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_time_n());
          }
        }
      }
      if (dual_time_1st){
      /*--- Set Solution at timestep n-1 to the previously loaded solution ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->GetSolution_Old());
            if (turbulent) {
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->GetSolution_Old());
            }
            if (heat) {
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_Old());
            }
          }
        }
      }
      if (dual_time_2nd){
        /*--- Set Solution at timestep n-1 to solution at n-2 ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->GetSolution_time_n1());
            if (turbulent) {
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->GetSolution_time_n1());
            }
            if (heat) {
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_time_n1());
            }
          }
        }
        /*--- Set Solution at timestep n-2 to the previously loaded solution ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {
            solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->Set_Solution_time_n1(solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->GetSolution_Old());
            if (turbulent) {
              solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->Set_Solution_time_n1(solver[val_iZone][val_iInst][iMesh][TURB_SOL]->node[iPoint]->GetSolution_Old());
            }
            if (heat) {
              solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n1(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_Old());
            }
          }
        }
      }
    }
  }

  /*--- Store flow solution also in the adjoint solver in order to be able to reset it later ---*/

  if (ExtIter == 0 || dual_time) {
    for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
      for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][iMesh]->GetnPoint(); iPoint++) {
        solver[val_iZone][val_iInst][iMesh][ADJFLOW_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->node[iPoint]->GetSolution());
      }
    }
    if (turbulent && !config[val_iZone]->GetFrozen_Visc_Disc()) {
      for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++) {
        solver[val_iZone][val_iInst][MESH_0][ADJTURB_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][MESH_0][TURB_SOL]->node[iPoint]->GetSolution());
      }
    }
    if (heat) {
      for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++) {
        solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][MESH_0][HEAT_SOL]->node[iPoint]->GetSolution());
      }
    }
  }

  solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->Preprocessing(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0],  config[val_iZone] , MESH_0, 0, RUNTIME_ADJFLOW_SYS, false);
  if (turbulent && !config[val_iZone]->GetFrozen_Visc_Disc()){
    solver[val_iZone][val_iInst][MESH_0][ADJTURB_SOL]->Preprocessing(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0],  config[val_iZone] , MESH_0, 0, RUNTIME_ADJTURB_SYS, false);
  }
  if (heat) {
    solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->Preprocessing(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0],  config[val_iZone] , MESH_0, 0, RUNTIME_ADJHEAT_SYS, false);
  }
}



void CDiscAdjFluidIteration::LoadUnsteady_Solution(CGeometry ****geometry,
                                           CSolver *****solver,
                                           CConfig **config,
                                           unsigned short val_iZone,
                                           unsigned short val_iInst, 
                                           int val_DirectIter) {
  unsigned short iMesh;
  bool heat = config[val_iZone]->GetWeakly_Coupled_Heat();

  if (val_DirectIter >= 0) {
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Loading flow solution from direct iteration " << val_DirectIter  << "." << endl;
    solver[val_iZone][val_iInst][MESH_0][FLOW_SOL]->LoadRestart(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], val_DirectIter, true);
    if (turbulent) {
      solver[val_iZone][val_iInst][MESH_0][TURB_SOL]->LoadRestart(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], val_DirectIter, false);
    }
    if (heat) {
      solver[val_iZone][val_iInst][MESH_0][HEAT_SOL]->LoadRestart(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], val_DirectIter, false);
    }
  } else {
    /*--- If there is no solution file we set the freestream condition ---*/
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Setting freestream conditions at direct iteration " << val_DirectIter << "." << endl;
    for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->SetFreeStream_Solution(config[val_iZone]);
      solver[val_iZone][val_iInst][iMesh][FLOW_SOL]->Preprocessing(geometry[val_iZone][val_iInst][iMesh],solver[val_iZone][val_iInst][iMesh], config[val_iZone], iMesh, val_DirectIter, RUNTIME_FLOW_SYS, false);
      if (turbulent) {
        solver[val_iZone][val_iInst][iMesh][TURB_SOL]->SetFreeStream_Solution(config[val_iZone]);
        solver[val_iZone][val_iInst][iMesh][TURB_SOL]->Postprocessing(geometry[val_iZone][val_iInst][iMesh],solver[val_iZone][val_iInst][iMesh], config[val_iZone], iMesh);
      }
      if (heat) {
        solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->SetFreeStream_Solution(config[val_iZone]);
        solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->Postprocessing(geometry[val_iZone][val_iInst][iMesh],solver[val_iZone][val_iInst][iMesh], config[val_iZone], iMesh);
      }
    }
  }
}


void CDiscAdjFluidIteration::Iterate(COutput *output,
                                        CIntegration ****integration,
                                        CGeometry ****geometry,
                                        CSolver *****solver,
                                        CNumerics ******numerics,
                                        CConfig **config,
                                        CSurfaceMovement **surface_movement,
                                        CVolumetricMovement ***volume_grid_movement,
                                        CFreeFormDefBox*** FFDBox,
                                        unsigned short val_iZone,
                                        unsigned short val_iInst) {
  
  unsigned long ExtIter = config[val_iZone]->GetExtIter();
  unsigned short Kind_Solver = config[val_iZone]->GetKind_Solver();
  unsigned long IntIter = 0;
  bool unsteady = config[val_iZone]->GetUnsteady_Simulation() != STEADY;
  bool frozen_visc = config[val_iZone]->GetFrozen_Visc_Disc();
  bool heat = config[val_iZone]->GetWeakly_Coupled_Heat();

  if (!unsteady)
    IntIter = ExtIter;
  else {
    IntIter = config[val_iZone]->GetIntIter();
  }

  /*--- Extract the adjoints of the conservative input variables and store them for the next iteration ---*/

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

    solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->ExtractAdjoint_Solution(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

    solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->ExtractAdjoint_Variables(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

    /*--- Set the convergence criteria (only residual possible) ---*/

    integration[val_iZone][val_iInst][ADJFLOW_SOL]->Convergence_Monitoring(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone],
                                                                          IntIter, log10(solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->GetRes_RMS(0)), MESH_0);

    }
  if (turbulent && !frozen_visc) {

    solver[val_iZone][val_iInst][MESH_0][ADJTURB_SOL]->ExtractAdjoint_Solution(geometry[val_iZone][val_iInst][MESH_0],
                                                                              config[val_iZone]);
  }
  if (heat) {

    solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->ExtractAdjoint_Solution(geometry[val_iZone][val_iInst][MESH_0],
                                                                              config[val_iZone]);
  }
}
  
    
void CDiscAdjFluidIteration::InitializeAdjoint(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();
  bool heat = config[iZone]->GetWeakly_Coupled_Heat();

  /*--- Initialize the adjoint of the objective function (typically with 1.0) ---*/
  
  solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdj_ObjFunc(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Initialize the adjoints the conservative variables ---*/

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

    solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);
  }

  if (turbulent && !frozen_visc) {
    solver[iZone][iInst][MESH_0][ADJTURB_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
        config[iZone]);
  }

  if (heat) {
    solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
        config[iZone]);
  }
}


void CDiscAdjFluidIteration::RegisterInput(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst, unsigned short kind_recording){

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();
  bool heat = config[iZone]->GetWeakly_Coupled_Heat();

  if (kind_recording == FLOW_CONS_VARS || kind_recording == COMBINED){
    
    /*--- Register flow and turbulent variables as input ---*/
    
    if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
        (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

      solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);

      solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->RegisterVariables(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
    
    if (turbulent && !frozen_visc) {
      solver[iZone][iInst][MESH_0][ADJTURB_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
    if (heat) {
      solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
  }
  if (kind_recording == MESH_COORDS){
    
    /*--- Register node coordinates as input ---*/
    
    geometry[iZone][iInst][MESH_0]->RegisterCoordinates(config[iZone]);
    
  }

  if (kind_recording == FLOW_CROSS_TERM){

    /*--- Register flow and turbulent variables as input ---*/

    solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);

    if (turbulent && !frozen_visc){
      solver[iZone][iInst][MESH_0][ADJTURB_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
  }

  if (kind_recording == GEOMETRY_CROSS_TERM){

    /*--- Register node coordinates as input ---*/

    geometry[iZone][iInst][MESH_0]->RegisterCoordinates(config[iZone]);

  }

}

void CDiscAdjFluidIteration::SetRecording(CSolver *****solver,
                                          CGeometry ****geometry,
                                          CConfig **config,
                                          unsigned short val_iZone,
                                          unsigned short val_iInst,
                                          unsigned short kind_recording) {

  unsigned short iMesh;

  /*--- Prepare for recording by resetting the solution to the initial converged solution ---*/

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

  for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++){
    solver[val_iZone][val_iInst][iMesh][ADJFLOW_SOL]->SetRecording(geometry[val_iZone][val_iInst][iMesh], config[val_iZone]);
  }
  if (config[val_iZone]->GetKind_Solver() == DISC_ADJ_RANS && !config[val_iZone]->GetFrozen_Visc_Disc()) {
    solver[val_iZone][val_iInst][MESH_0][ADJTURB_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);
  }
  if (config[val_iZone]->GetWeakly_Coupled_Heat()) {
    solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);
  }


}

void CDiscAdjFluidIteration::SetDependencies(CSolver *****solver,
                                             CGeometry ****geometry,
                                             CNumerics ******numerics,
                                             CConfig **config,
                                             unsigned short iZone,
                                             unsigned short iInst,
                                             unsigned short kind_recording){

  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();
  bool heat = config[iZone]->GetWeakly_Coupled_Heat();
  if ((kind_recording == MESH_COORDS) || (kind_recording == NONE)  ||
      (kind_recording == GEOMETRY_CROSS_TERM) || (kind_recording == ALL_VARIABLES)){

    /*--- Update geometry to get the influence on other geometry variables (normals, volume etc) ---*/

    geometry[iZone][iInst][MESH_0]->UpdateGeometry(geometry[iZone][iInst], config[iZone]);

  }

  /*--- Compute coupling between flow and turbulent equations ---*/

  solver[iZone][iInst][MESH_0][FLOW_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  solver[iZone][iInst][MESH_0][FLOW_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);

  if (turbulent && !frozen_visc){
    solver[iZone][iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, true);
    solver[iZone][iInst][MESH_0][TURB_SOL]->Postprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0);
    solver[iZone][iInst][MESH_0][TURB_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
    solver[iZone][iInst][MESH_0][TURB_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);

  }

  if (heat){
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Set_Heatflux_Areas(geometry[iZone][iInst][MESH_0], config[iZone]);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Preprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0, NO_RK_ITER, RUNTIME_HEAT_SYS, true);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Postprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  }
}

void CDiscAdjFluidIteration::RegisterOutput(CSolver *****solver, CGeometry ****geometry, CConfig **config, COutput* output, unsigned short iZone, unsigned short iInst){
  
  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();
  bool heat = config[iZone]->GetWeakly_Coupled_Heat();

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {
  
  /*--- Register conservative variables as output of the iteration ---*/
  
    solver[iZone][iInst][MESH_0][FLOW_SOL]->RegisterOutput(geometry[iZone][iInst][MESH_0],config[iZone]);
  
  }
  if (turbulent && !frozen_visc){
    solver[iZone][iInst][MESH_0][TURB_SOL]->RegisterOutput(geometry[iZone][iInst][MESH_0],
                                                                 config[iZone]);
  }
  if (heat){
    solver[iZone][iInst][MESH_0][HEAT_SOL]->RegisterOutput(geometry[iZone][iInst][MESH_0],
                                                                 config[iZone]);
  }
}

void CDiscAdjFluidIteration::InitializeAdjoint_CrossTerm(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();

  /*--- Initialize the adjoint of the objective function (typically with 1.0) ---*/

  solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdj_ObjFunc(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Initialize the adjoints the conservative variables ---*/

 if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER)) {

  solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);
}

  if (turbulent && !frozen_visc) {
    solver[iZone][iInst][MESH_0][ADJTURB_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                    config[iZone]);
  }
}


void CDiscAdjFluidIteration::Update(COutput *output,
                                       CIntegration ****integration,
                                       CGeometry ****geometry,
                                       CSolver *****solver,
                                       CNumerics ******numerics,
                                       CConfig **config,
                                       CSurfaceMovement **surface_movement,
                                       CVolumetricMovement ***grid_movement,
                                       CFreeFormDefBox*** FFDBox,
                                       unsigned short val_iZone,
                                       unsigned short val_iInst)      {

  unsigned short iMesh;

  /*--- Dual time stepping strategy ---*/

  if ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
      (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {

    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      integration[val_iZone][val_iInst][ADJFLOW_SOL]->SetConvergence(false);
    }
  }
}
bool CDiscAdjFluidIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     {

  bool StopCalc = false;
  bool steady = (config[val_iZone]->GetUnsteady_Simulation() == STEADY);
  bool output_history = false;

#ifndef HAVE_MPI
  StopTime = su2double(clock())/su2double(CLOCKS_PER_SEC);
#else
  StopTime = MPI_Wtime();
#endif
  UsedTime = StopTime - StartTime;

  /*--- If convergence was reached --*/
  StopCalc = integration[val_iZone][INST_0][ADJFLOW_SOL]->GetConvergence();

  /*--- Write the convergence history for the fluid (only screen output) ---*/

  /*--- The logic is right now case dependent ----*/
  /*--- This needs to be generalized when the new output structure comes ---*/
  output_history = (steady && !(multizone && (config[val_iZone]->GetnInner_Iter()==1)));

  if (output_history) output->SetConvHistory_Body(NULL, geometry, solver, config, integration, false, UsedTime, val_iZone, INST_0);

  return StopCalc;

}
void CDiscAdjFluidIteration::Postprocess(COutput *output,
                                         CIntegration ****integration,
                                         CGeometry ****geometry,
                                         CSolver *****solver,
                                         CNumerics ******numerics,
                                         CConfig **config,
                                         CSurfaceMovement **surface_movement,
                                         CVolumetricMovement ***grid_movement,
                                         CFreeFormDefBox*** FFDBox,
                                         unsigned short val_iZone,
                                         unsigned short val_iInst) { }


CDiscAdjFEAIteration::CDiscAdjFEAIteration(CConfig *config) : CIteration(config), CurrentRecording(NONE){

  fem_iteration = new CFEAIteration(config);

  // TEMPORARY output only for standalone structural problems
  if ((!config->GetFSI_Simulation()) && (rank == MASTER_NODE)){

    bool de_effects = config->GetDE_Effects();
    unsigned short iVar;

    /*--- Header of the temporary output file ---*/
    ofstream myfile_res;
    myfile_res.open ("Results_Reverse_Adjoint.txt");

    myfile_res << "Obj_Func" << " ";
    for (iVar = 0; iVar < config->GetnElasticityMod(); iVar++)
        myfile_res << "Sens_E_" << iVar << "\t";

    for (iVar = 0; iVar < config->GetnPoissonRatio(); iVar++)
      myfile_res << "Sens_Nu_" << iVar << "\t";

    if (config->GetDynamic_Analysis() == DYNAMIC){
        for (iVar = 0; iVar < config->GetnMaterialDensity(); iVar++)
          myfile_res << "Sens_Rho_" << iVar << "\t";
    }

    if (de_effects){
        for (iVar = 0; iVar < config->GetnElectric_Field(); iVar++)
          myfile_res << "Sens_EField_" << iVar << "\t";
    }

    myfile_res << endl;

    myfile_res.close();
  }

}

CDiscAdjFEAIteration::~CDiscAdjFEAIteration(void) { }
void CDiscAdjFEAIteration::Preprocess(COutput *output,
                                           CIntegration ****integration,
                                           CGeometry ****geometry,
                                           CSolver *****solver,
                                           CNumerics ******numerics,
                                           CConfig **config,
                                           CSurfaceMovement **surface_movement,
                                           CVolumetricMovement ***grid_movement,
                                           CFreeFormDefBox*** FFDBox,
                                           unsigned short val_iZone,
                                           unsigned short val_iInst) {

  unsigned long IntIter = 0, iPoint;
  config[ZONE_0]->SetIntIter(IntIter);
  unsigned short ExtIter = config[val_iZone]->GetExtIter();
  bool dynamic = (config[val_iZone]->GetDynamic_Analysis() == DYNAMIC);

  int Direct_Iter;

  /*--- For the dynamic adjoint, load direct solutions from restart files. ---*/

  if (dynamic) {

    Direct_Iter = SU2_TYPE::Int(config[val_iZone]->GetUnst_AdjointIter()) - SU2_TYPE::Int(ExtIter) - 1;

    /*--- We want to load the already converged solution at timesteps n and n-1 ---*/

    /*--- Load solution at timestep n-1 ---*/

    LoadDynamic_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter-1);

    /*--- Push solution back to correct array ---*/

    for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][MESH_0]->GetnPoint();iPoint++){
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution_time_n();
    }

    /*--- Push solution back to correct array ---*/

    for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][MESH_0]->GetnPoint();iPoint++){
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution_Accel_time_n();
    }

    /*--- Push solution back to correct array ---*/

    for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][MESH_0]->GetnPoint();iPoint++){
      solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution_Vel_time_n();
    }

    /*--- Load solution timestep n ---*/

    LoadDynamic_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter);

    /*--- Store FEA solution also in the adjoint solver in order to be able to reset it later ---*/

    for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++){
      solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->GetSolution());
    }

    for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++){
      solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->node[iPoint]->SetSolution_Accel_Direct(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->GetSolution_Accel());
    }

    for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++){
      solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->node[iPoint]->SetSolution_Vel_Direct(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->GetSolution_Vel());
    }

  }
  else{
    /*--- Store FEA solution also in the adjoint solver in order to be able to reset it later ---*/

    for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++){
      solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->GetSolution());
    }

  }

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->Preprocessing(geometry[val_iZone][val_iInst][MESH_0], solver[val_iZone][val_iInst][MESH_0],  config[val_iZone] , MESH_0, 0, RUNTIME_ADJFEA_SYS, false);

}



void CDiscAdjFEAIteration::LoadDynamic_Solution(CGeometry ****geometry,
                                               CSolver *****solver,
                                               CConfig **config,
                                               unsigned short val_iZone,
                                               unsigned short val_iInst, 
                                               int val_DirectIter) {
  unsigned short iVar;
  unsigned long iPoint;
  bool update_geo = false;  //TODO: check

  if (val_DirectIter >= 0){
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Loading FEA solution from direct iteration " << val_DirectIter  << "." << endl;
    solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->LoadRestart(geometry[val_iZone][val_iInst], solver[val_iZone][val_iInst], config[val_iZone], val_DirectIter, update_geo);
  } else {
    /*--- If there is no solution file we set the freestream condition ---*/
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Setting static conditions at direct iteration " << val_DirectIter << "." << endl;
    /*--- Push solution back to correct array ---*/
    for(iPoint=0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint();iPoint++){
      for (iVar = 0; iVar < solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetnVar(); iVar++){
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution(iVar, 0.0);
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution_Accel(iVar, 0.0);
        solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->node[iPoint]->SetSolution_Vel(iVar, 0.0);
      }
    }
  }
}


void CDiscAdjFEAIteration::Iterate(COutput *output,
                                        CIntegration ****integration,
                                        CGeometry ****geometry,
                                        CSolver *****solver,
                                        CNumerics ******numerics,
                                        CConfig **config,
                                        CSurfaceMovement **surface_movement,
                                        CVolumetricMovement ***volume_grid_movement,
                                        CFreeFormDefBox*** FFDBox,
                                        unsigned short val_iZone,
                                        unsigned short val_iInst) {


  bool dynamic = (config[val_iZone]->GetDynamic_Analysis() == DYNAMIC);

  unsigned long nIntIter = config[val_iZone]->GetnIter();
  unsigned long IntIter  = config[val_iZone]->GetIntIter();

  /*--- Extract the adjoints of the conservative input variables and store them for the next iteration ---*/

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->ExtractAdjoint_Solution(geometry[val_iZone][val_iInst][MESH_0],
                                                                                      config[val_iZone]);

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->ExtractAdjoint_Variables(geometry[val_iZone][val_iInst][MESH_0],
                                                                                       config[val_iZone]);

  /*--- Set the convergence criteria (only residual possible) ---*/

  integration[val_iZone][val_iInst][ADJFEA_SOL]->Convergence_Monitoring(geometry[val_iZone][val_iInst][MESH_0],config[val_iZone],
                                                                                  IntIter,log10(solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->GetRes_RMS(0)), MESH_0);

  /*--- Write the convergence history (only screen output) ---*/

  if(IntIter != nIntIter-1)
    output->SetConvHistory_Body(NULL, geometry, solver, config, integration, true, 0.0, val_iZone, val_iInst);

  if (dynamic){
    integration[val_iZone][val_iInst][ADJFEA_SOL]->SetConvergence(false);
  }

}

void CDiscAdjFEAIteration::SetRecording(COutput *output,
                                             CIntegration ****integration,
                                             CGeometry ****geometry,
                                             CSolver *****solver,
                                             CNumerics ******numerics,
                                             CConfig **config,
                                             CSurfaceMovement **surface_movement,
                                             CVolumetricMovement ***grid_movement,
                                             CFreeFormDefBox*** FFDBox,
                                             unsigned short val_iZone,
                                             unsigned short val_iInst,
                                             unsigned short kind_recording)      {

  unsigned long IntIter = config[ZONE_0]->GetIntIter();
  unsigned long ExtIter = config[val_iZone]->GetExtIter(), DirectExtIter;
  bool dynamic = (config[val_iZone]->GetDynamic_Analysis() == DYNAMIC);

  DirectExtIter = 0;
  if (dynamic){
    DirectExtIter = SU2_TYPE::Int(config[val_iZone]->GetUnst_AdjointIter()) - SU2_TYPE::Int(ExtIter) - 1;
  }

  /*--- Reset the tape ---*/

  AD::Reset();

  /*--- We only need to reset the indices if the current recording is different from the recording we want to have ---*/

  if (CurrentRecording != kind_recording && (CurrentRecording != NONE) ){

    solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

    /*--- Clear indices of coupling variables ---*/

    SetDependencies(solver, geometry, numerics, config, val_iZone, val_iInst, ALL_VARIABLES);

    /*--- Run one iteration while tape is passive - this clears all indices ---*/

    fem_iteration->Iterate(output,integration,geometry,solver,numerics,
                                config,surface_movement,grid_movement,FFDBox,val_iZone, val_iInst);

  }

  /*--- Prepare for recording ---*/

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

  /*--- Start the recording of all operations ---*/

  AD::StartRecording();

  /*--- Register FEA variables ---*/

  RegisterInput(solver, geometry, config, val_iZone, val_iInst, kind_recording);

  /*--- Compute coupling or update the geometry ---*/

  SetDependencies(solver, geometry, numerics, config, val_iZone, val_iInst, kind_recording);

  /*--- Set the correct direct iteration number ---*/

  if (dynamic){
    config[val_iZone]->SetExtIter(DirectExtIter);
  }

  /*--- Run the direct iteration ---*/

  fem_iteration->Iterate(output,integration,geometry,solver,numerics,
                              config,surface_movement,grid_movement,FFDBox, val_iZone, val_iInst);

  config[val_iZone]->SetExtIter(ExtIter);

  /*--- Register structural variables and objective function as output ---*/

  RegisterOutput(solver, geometry, config, val_iZone, val_iInst);

  /*--- Stop the recording ---*/

  AD::StopRecording();

  /*--- Set the recording status ---*/

  CurrentRecording = kind_recording;

  /* --- Reset the number of the internal iterations---*/

  config[ZONE_0]->SetIntIter(IntIter);

}


void CDiscAdjFEAIteration::SetRecording(CSolver *****solver,
                                        CGeometry ****geometry,
                                        CConfig **config,
                                        unsigned short val_iZone,
                                        unsigned short val_iInst,
                                        unsigned short kind_recording) {

  /*--- Prepare for recording by resetting the solution to the initial converged solution ---*/

  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->SetRecording(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

}

void CDiscAdjFEAIteration::RegisterInput(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst, unsigned short kind_recording){

  /*--- Register structural displacements as input ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Register variables as input ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->RegisterVariables(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Both need to be registered regardless of kind_recording for structural shape derivatives to work properly.
        Otherwise, the code simply diverges as the FEM_CROSS_TERM_GEOMETRY breaks! (no idea why) for this term we register but do not extract! ---*/
}

void CDiscAdjFEAIteration::SetDependencies(CSolver *****solver, CGeometry ****geometry, CNumerics ******numerics, CConfig **config, unsigned short iZone, unsigned short iInst, unsigned short kind_recording){

  unsigned short iVar;
  unsigned short iMPROP = config[iZone]->GetnElasticityMod();
  
  /*--- Some numerics are only instanciated under these conditions ---*/
  bool element_based = (config[iZone]->GetGeometricConditions() == LARGE_DEFORMATIONS) &&
                        solver[iZone][iInst][MESH_0][FEA_SOL]->IsElementBased(),
       de_effects    = (config[iZone]->GetGeometricConditions() == LARGE_DEFORMATIONS) &&
                        config[iZone]->GetDE_Effects();

  for (iVar = 0; iVar < iMPROP; iVar++){

      /*--- Add dependencies for E and Nu ---*/

      numerics[iZone][iInst][MESH_0][FEA_SOL][FEA_TERM]->SetMaterial_Properties(iVar,
                                                                                   solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Young(iVar),
                                                                                   solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Poisson(iVar));

      /*--- Add dependencies for Rho and Rho_DL ---*/

      numerics[iZone][iInst][MESH_0][FEA_SOL][FEA_TERM]->SetMaterial_Density(iVar,
                                                                                solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho(iVar),
                                                                                solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho_DL(iVar));

      /*--- Add dependencies for element-based simulations. ---*/

      if (element_based){

          /*--- Neo Hookean Compressible ---*/
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_NHCOMP]->SetMaterial_Properties(iVar,
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Young(iVar),
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Poisson(iVar));
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_NHCOMP]->SetMaterial_Density(iVar,
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho(iVar),
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho_DL(iVar));

          /*--- Ideal DE ---*/
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_IDEALDE]->SetMaterial_Properties(iVar,
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Young(iVar),
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Poisson(iVar));
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_IDEALDE]->SetMaterial_Density(iVar,
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho(iVar),
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho_DL(iVar));

          /*--- Knowles ---*/
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_KNOWLES]->SetMaterial_Properties(iVar,
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Young(iVar),
                                                                                       solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Poisson(iVar));
          numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_KNOWLES]->SetMaterial_Density(iVar,
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho(iVar),
                                                                                    solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_Rho_DL(iVar));

      }



  }

  if (de_effects){

      unsigned short nEField = solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetnEField();

      for (unsigned short iEField = 0; iEField < nEField; iEField++){

          numerics[iZone][iInst][MESH_0][FEA_SOL][FEA_TERM]->Set_ElectricField(iEField,
                                                                                 solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_EField(iEField));

          numerics[iZone][iInst][MESH_0][FEA_SOL][DE_TERM]->Set_ElectricField(iEField,
                                                                                 solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_EField(iEField));

      }


  }

  /*--- Add dependencies for element-based simulations. ---*/

  switch (config[iZone]->GetDV_FEA()) {
    case YOUNG_MODULUS:
    case POISSON_RATIO:
    case DENSITY_VAL:
    case DEAD_WEIGHT:
    case ELECTRIC_FIELD:

      unsigned short nDV = solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetnDVFEA();

      for (unsigned short iDV = 0; iDV < nDV; iDV++){

          numerics[iZone][iInst][MESH_0][FEA_SOL][FEA_TERM]->Set_DV_Val(iDV,
                                                                           solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_DVFEA(iDV));

          if (de_effects){
            numerics[iZone][iInst][MESH_0][FEA_SOL][DE_TERM]->Set_DV_Val(iDV,
                                                                            solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_DVFEA(iDV));
          }

      }

      if (element_based){

        for (unsigned short iDV = 0; iDV < nDV; iDV++){
            numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_NHCOMP]->Set_DV_Val(iDV,
                                                                            solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_DVFEA(iDV));
            numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_IDEALDE]->Set_DV_Val(iDV,
                                                                            solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_DVFEA(iDV));
            numerics[iZone][iInst][MESH_0][FEA_SOL][MAT_KNOWLES]->Set_DV_Val(iDV,
                                                                            solver[iZone][iInst][MESH_0][ADJFEA_SOL]->GetVal_DVFEA(iDV));
        }

      }

    break;

  }

}

void CDiscAdjFEAIteration::RegisterOutput(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  /*--- Register conservative variables as output of the iteration ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->RegisterOutput(geometry[iZone][iInst][MESH_0],config[iZone]);

}

void CDiscAdjFEAIteration::InitializeAdjoint(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  /*--- Initialize the adjoint of the objective function (typically with 1.0) ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->SetAdj_ObjFunc(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Initialize the adjoints the conservative variables ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);

}


void CDiscAdjFEAIteration::InitializeAdjoint_CrossTerm(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  /*--- Initialize the adjoint of the objective function (typically with 1.0) ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->SetAdj_ObjFunc(geometry[iZone][iInst][MESH_0], config[iZone]);

  /*--- Initialize the adjoints the conservative variables ---*/

  solver[iZone][iInst][MESH_0][ADJFEA_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);

}

void CDiscAdjFEAIteration::Update(COutput *output,
                                       CIntegration ****integration,
                                       CGeometry ****geometry,
                                       CSolver *****solver,
                                       CNumerics ******numerics,
                                       CConfig **config,
                                       CSurfaceMovement **surface_movement,
                                       CVolumetricMovement ***grid_movement,
                                       CFreeFormDefBox*** FFDBox,
                                       unsigned short val_iZone,
                                       unsigned short val_iInst)      { }
bool CDiscAdjFEAIteration::Monitor(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst)     { return false; }
void CDiscAdjFEAIteration::Postprocess(COutput *output,
    CIntegration ****integration,
    CGeometry ****geometry,
    CSolver *****solver,
    CNumerics ******numerics,
    CConfig **config,
    CSurfaceMovement **surface_movement,
    CVolumetricMovement ***grid_movement,
    CFreeFormDefBox*** FFDBox,
    unsigned short val_iZone,
    unsigned short val_iInst) {


  bool dynamic = (config[val_iZone]->GetDynamic_Analysis() == DYNAMIC);

  /*--- Global sensitivities ---*/
  solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->SetSensitivity(geometry[val_iZone][val_iInst][MESH_0],config[val_iZone]);

  // TEMPORARY output only for standalone structural problems
  if ((!config[val_iZone]->GetFSI_Simulation()) && (rank == MASTER_NODE)){

    unsigned short iVar;

    bool de_effects = config[val_iZone]->GetDE_Effects();

    /*--- Header of the temporary output file ---*/
    ofstream myfile_res;
    myfile_res.open ("Results_Reverse_Adjoint.txt", ios::app);

    myfile_res.precision(15);

    myfile_res << config[val_iZone]->GetExtIter() << "\t";

    switch (config[val_iZone]->GetKind_ObjFunc()){
    case REFERENCE_GEOMETRY:
      myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetTotal_OFRefGeom() << "\t";
      break;
    case REFERENCE_NODE:
      myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetTotal_OFRefNode() << "\t";
      break;
    case VOLUME_FRACTION:
      myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][FEA_SOL]->GetTotal_OFVolFrac() << "\t";
      break;
    }

    for (iVar = 0; iVar < config[val_iZone]->GetnElasticityMod(); iVar++)
      myfile_res << scientific << solver[ZONE_0][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_E(iVar) << "\t";
    for (iVar = 0; iVar < config[val_iZone]->GetnPoissonRatio(); iVar++)
      myfile_res << scientific << solver[ZONE_0][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_Nu(iVar) << "\t";
    if (dynamic){
      for (iVar = 0; iVar < config[val_iZone]->GetnMaterialDensity(); iVar++)
        myfile_res << scientific << solver[ZONE_0][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_Rho(iVar) << "\t";
    }

    if (de_effects){
      for (iVar = 0; iVar < config[val_iZone]->GetnElectric_Field(); iVar++)
        myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_EField(iVar) << "\t";
    }

    for (iVar = 0; iVar < solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->GetnDVFEA(); iVar++){
      myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_DVFEA(iVar) << "\t";
    }

    myfile_res << endl;

    myfile_res.close();
  }

  // TEST: for implementation of python framework in standalone structural problems
  if ((!config[val_iZone]->GetFSI_Simulation()) && (rank == MASTER_NODE)){

    /*--- Header of the temporary output file ---*/
    ofstream myfile_res;
    bool outputDVFEA = false;

    switch (config[val_iZone]->GetDV_FEA()) {
    case YOUNG_MODULUS:
      myfile_res.open("grad_young.opt");
      outputDVFEA = true;
      break;
    case POISSON_RATIO:
      myfile_res.open("grad_poisson.opt");
      outputDVFEA = true;
      break;
    case DENSITY_VAL:
    case DEAD_WEIGHT:
      myfile_res.open("grad_density.opt");
      outputDVFEA = true;
      break;
    case ELECTRIC_FIELD:
      myfile_res.open("grad_efield.opt");
      outputDVFEA = true;
      break;
    default:
      outputDVFEA = false;
      break;
    }

    if (outputDVFEA){

      unsigned short iDV;
      unsigned short nDV = solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->GetnDVFEA();

      myfile_res << "INDEX" << "\t" << "GRAD" << endl;

      myfile_res.precision(15);

      for (iDV = 0; iDV < nDV; iDV++){
        myfile_res << iDV;
        myfile_res << "\t";
        myfile_res << scientific << solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->GetTotal_Sens_DVFEA(iDV);
        myfile_res << endl;
      }

      myfile_res.close();

    }

  }

  unsigned short iMarker;

  /*--- Apply BC's to the structural adjoint - otherwise, clamped nodes have too values that make no sense... ---*/
  for (iMarker = 0; iMarker < config[val_iZone]->GetnMarker_All(); iMarker++)
  switch (config[val_iZone]->GetMarker_All_KindBC(iMarker)) {
    case CLAMPED_BOUNDARY:
    solver[val_iZone][val_iInst][MESH_0][ADJFEA_SOL]->BC_Clamped_Post(geometry[val_iZone][val_iInst][MESH_0],
        solver[val_iZone][val_iInst][MESH_0], numerics[val_iZone][val_iInst][MESH_0][FEA_SOL][FEA_TERM],
        config[val_iZone], iMarker);
    break;
  }
}

CDiscAdjHeatIteration::CDiscAdjHeatIteration(CConfig *config) : CIteration(config) { }

CDiscAdjHeatIteration::~CDiscAdjHeatIteration(void) { }

void CDiscAdjHeatIteration::Preprocess(COutput *output,
                                           CIntegration ****integration,
                                           CGeometry ****geometry,
                                           CSolver *****solver,
                                           CNumerics ******numerics,
                                           CConfig **config,
                                           CSurfaceMovement **surface_movement,
                                           CVolumetricMovement ***grid_movement,
                                           CFreeFormDefBox*** FFDBox,
                                           unsigned short val_iZone,
                                           unsigned short val_iInst) {

  unsigned long IntIter = 0, iPoint;
  config[ZONE_0]->SetIntIter(IntIter);
  unsigned short ExtIter = config[val_iZone]->GetExtIter();
  bool dual_time_1st = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST);
  bool dual_time_2nd = (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND);
  bool dual_time = (dual_time_1st || dual_time_2nd);
  unsigned short iMesh;
  int Direct_Iter;

  /*--- For the unsteady adjoint, load direct solutions from restart files. ---*/

  if (config[val_iZone]->GetUnsteady_Simulation()) {

    Direct_Iter = SU2_TYPE::Int(config[val_iZone]->GetUnst_AdjointIter()) - SU2_TYPE::Int(ExtIter) - 2;

    /*--- For dual-time stepping we want to load the already converged solution at timestep n ---*/

    if (dual_time) {
      Direct_Iter += 1;
    }

    if (ExtIter == 0){

      if (dual_time_2nd) {

        /*--- Load solution at timestep n-2 ---*/

        LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter-2);

        /*--- Push solution back to correct array ---*/

        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n();
            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n1();
          }
        }
      }
      if (dual_time) {

        /*--- Load solution at timestep n-1 ---*/

        LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter-1);

        /*--- Push solution back to correct array ---*/

        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n();
          }
        }
      }

      /*--- Load solution timestep n ---*/

      LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter);

    }


    if ((ExtIter > 0) && dual_time){

      /*--- Load solution timestep n - 2 ---*/

      LoadUnsteady_Solution(geometry, solver,config, val_iZone, val_iInst, Direct_Iter - 2);

      /*--- Temporarily store the loaded solution in the Solution_Old array ---*/

      for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
        for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

          solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_OldSolution();
        }
      }

      /*--- Set Solution at timestep n to solution at n-1 ---*/

      for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
        for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

          solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->SetSolution(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_time_n());
        }
      }
      if (dual_time_1st){
      /*--- Set Solution at timestep n-1 to the previously loaded solution ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_time_n1());
          }
        }
      }
      if (dual_time_2nd){
        /*--- Set Solution at timestep n-1 to solution at n-2 ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_time_n1());
          }
        }
        /*--- Set Solution at timestep n-2 to the previously loaded solution ---*/
        for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {
          for(iPoint=0; iPoint<geometry[val_iZone][val_iInst][iMesh]->GetnPoint();iPoint++) {

            solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->Set_Solution_time_n1(solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->node[iPoint]->GetSolution_Old());
          }
        }
      }
    }
  }

  /*--- Store flow solution also in the adjoint solver in order to be able to reset it later ---*/

  if (ExtIter == 0 || dual_time) {
    for (iPoint = 0; iPoint < geometry[val_iZone][val_iInst][MESH_0]->GetnPoint(); iPoint++) {
      solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->node[iPoint]->SetSolution_Direct(solver[val_iZone][val_iInst][MESH_0][HEAT_SOL]->node[iPoint]->GetSolution());
    }
  }

  solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->Preprocessing(geometry[val_iZone][val_iInst][MESH_0],
                                                                             solver[val_iZone][val_iInst][MESH_0],
                                                                             config[val_iZone],
                                                                             MESH_0, 0, RUNTIME_ADJHEAT_SYS, false);
}



void CDiscAdjHeatIteration::LoadUnsteady_Solution(CGeometry ****geometry,
                                           CSolver *****solver,
                                           CConfig **config,
                                           unsigned short val_iZone,
                                           unsigned short val_iInst,
                                           int val_DirectIter) {
  unsigned short iMesh;

  if (val_DirectIter >= 0) {
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Loading heat solution from direct iteration " << val_DirectIter  << "." << endl;

    solver[val_iZone][val_iInst][MESH_0][HEAT_SOL]->LoadRestart(geometry[val_iZone][val_iInst],
                                                                          solver[val_iZone][val_iInst],
                                                                          config[val_iZone],
                                                                          val_DirectIter, false);
  }

  else {
    /*--- If there is no solution file we set the freestream condition ---*/
    if (rank == MASTER_NODE && val_iZone == ZONE_0)
      cout << " Setting freestream conditions at direct iteration " << val_DirectIter << "." << endl;
    for (iMesh=0; iMesh<=config[val_iZone]->GetnMGLevels();iMesh++) {

      solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->SetFreeStream_Solution(config[val_iZone]);
      solver[val_iZone][val_iInst][iMesh][HEAT_SOL]->Postprocessing(geometry[val_iZone][val_iInst][iMesh],
                                                                              solver[val_iZone][val_iInst][iMesh],
                                                                              config[val_iZone],
                                                                              iMesh);
    }
  }
}


void CDiscAdjHeatIteration::Iterate(COutput *output,
                                        CIntegration ****integration,
                                        CGeometry ****geometry,
                                        CSolver *****solver,
                                        CNumerics ******numerics,
                                        CConfig **config,
                                        CSurfaceMovement **surface_movement,
                                        CVolumetricMovement ***volume_grid_movement,
                                        CFreeFormDefBox*** FFDBox,
                                        unsigned short val_iZone,
                                        unsigned short val_iInst) {


  solver[val_iZone][val_iInst][MESH_0][ADJHEAT_SOL]->ExtractAdjoint_Solution(geometry[val_iZone][val_iInst][MESH_0],
                                                                                       config[val_iZone]);
}

void CDiscAdjHeatIteration::InitializeAdjoint(CSolver *****solver,
                                              CGeometry ****geometry,
                                              CConfig **config,
                                              unsigned short iZone, unsigned short iInst){

  /*--- Initialize the adjoints the conservative variables ---*/

  solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->SetAdjoint_Output(geometry[iZone][iInst][MESH_0],
                                                                         config[iZone]);
}


void CDiscAdjHeatIteration::RegisterInput(CSolver *****solver,
                                          CGeometry ****geometry,
                                          CConfig **config,
                                          unsigned short iZone, unsigned short iInst,
                                          unsigned short kind_recording){

  if (kind_recording == FLOW_CONS_VARS || kind_recording == COMBINED){

    /*--- Register flow and turbulent variables as input ---*/

    solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);

    solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->RegisterVariables(geometry[iZone][iInst][MESH_0], config[iZone]);

  }
  if (kind_recording == MESH_COORDS){

    /*--- Register node coordinates as input ---*/

    geometry[iZone][iInst][MESH_0]->RegisterCoordinates(config[iZone]);

  }
}

void CDiscAdjHeatIteration::SetDependencies(CSolver *****solver,
                                            CGeometry ****geometry,
                                            CNumerics ******numerics,
                                            CConfig **config,
                                            unsigned short iZone, unsigned short iInst,
                                            unsigned short kind_recording){

  if ((kind_recording == MESH_COORDS) || (kind_recording == NONE)  ||
      (kind_recording == GEOMETRY_CROSS_TERM) || (kind_recording == ALL_VARIABLES)){

    /*--- Update geometry to get the influence on other geometry variables (normals, volume etc) ---*/

    geometry[iZone][iInst][MESH_0]->UpdateGeometry(geometry[iZone][iInst], config[iZone]);

  }

  solver[iZone][iInst][MESH_0][HEAT_SOL]->Set_Heatflux_Areas(geometry[iZone][iInst][MESH_0], config[iZone]);
  solver[iZone][iInst][MESH_0][HEAT_SOL]->Preprocessing(geometry[iZone][iInst][MESH_0], solver[iZone][iInst][MESH_0],
                                                                  config[iZone], MESH_0, NO_RK_ITER, RUNTIME_HEAT_SYS, true);
  solver[iZone][iInst][MESH_0][HEAT_SOL]->Postprocessing(geometry[iZone][iInst][MESH_0], solver[iZone][iInst][MESH_0],
                                                                   config[iZone], MESH_0);  
  solver[iZone][iInst][MESH_0][HEAT_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  solver[iZone][iInst][MESH_0][HEAT_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  
}

void CDiscAdjHeatIteration::RegisterOutput(CSolver *****solver,
                                           CGeometry ****geometry,
                                           CConfig **config, COutput* output,
                                           unsigned short iZone, unsigned short iInst){

  solver[iZone][iInst][MESH_0][ADJHEAT_SOL]->RegisterOutput(geometry[iZone][iInst][MESH_0], config[iZone]);

  geometry[iZone][iInst][MESH_0]->RegisterOutput_Coordinates(config[iZone]);
}

void CDiscAdjHeatIteration::Update(COutput *output,
                                       CIntegration ****integration,
                                       CGeometry ****geometry,
                                       CSolver *****solver,
                                       CNumerics ******numerics,
                                       CConfig **config,
                                       CSurfaceMovement **surface_movement,
                                       CVolumetricMovement ***grid_movement,
                                       CFreeFormDefBox*** FFDBox,
                                       unsigned short val_iZone, unsigned short val_iInst)      {

  unsigned short iMesh;

  /*--- Dual time stepping strategy ---*/

  if ((config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
      (config[val_iZone]->GetUnsteady_Simulation() == DT_STEPPING_2ND)) {

    for (iMesh = 0; iMesh <= config[val_iZone]->GetnMGLevels(); iMesh++) {
      integration[val_iZone][val_iInst][ADJHEAT_SOL]->SetConvergence(false);
    }
  }
}

bool CDiscAdjHeatIteration::Monitor(COutput *output,
                                    CIntegration ****integration,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CNumerics ******numerics,
                                    CConfig **config,
                                    CSurfaceMovement **surface_movement,
                                    CVolumetricMovement ***grid_movement,
                                    CFreeFormDefBox*** FFDBox,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) { return false; }


void  CDiscAdjHeatIteration::Output(COutput *output,
                                    CGeometry ****geometry,
                                    CSolver *****solver,
                                    CConfig **config,
                                    unsigned long ExtIter,
                                    bool StopCalc,
                                    unsigned short val_iZone,
                                    unsigned short val_iInst) { }

void CDiscAdjHeatIteration::Postprocess(COutput *output,
                                         CIntegration ****integration,
                                         CGeometry ****geometry,
                                         CSolver *****solver,
                                         CNumerics ******numerics,
                                         CConfig **config,
                                         CSurfaceMovement **surface_movement,
                                         CVolumetricMovement ***grid_movement,
                                         CFreeFormDefBox*** FFDBox,
                                         unsigned short val_iZone, unsigned short val_iInst) { }

COneShotFluidIteration::COneShotFluidIteration(CConfig *config) : CDiscAdjFluidIteration(config) {

  turbulent = ( config->GetKind_Solver() == ONE_SHOT_RANS );

}

COneShotFluidIteration::~COneShotFluidIteration(void) { }

void COneShotFluidIteration::RegisterInput(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst, unsigned short kind_recording){

  /*--- For the one-shot strategy conservative variables as well as mesh coordinates are recorded. Furthermore, we need to record the mesh coordinates in every flow iteration,
   *  thus we make use of the COMBINED recording in each step ---*/

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();

  if (kind_recording == FLOW_CONS_VARS || kind_recording == COMBINED){

    /*--- Register flow and turbulent variables as input ---*/

    if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
        (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

      solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);

      solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->RegisterVariables(geometry[iZone][iInst][MESH_0], config[iZone]);
    }

    if (((Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == ONE_SHOT_RANS)) && !frozen_visc) {
      solver[iZone][iInst][MESH_0][ADJTURB_SOL]->RegisterSolution(geometry[iZone][iInst][MESH_0], config[iZone]);
    }
  }

  if (kind_recording == MESH_COORDS || kind_recording == COMBINED){

    /*--- Register node coordinates as input ---*/

    geometry[iZone][iInst][MESH_0]->RegisterCoordinates(config[iZone]);

  }

}

void COneShotFluidIteration::SetDependencies(CSolver *****solver, CGeometry ****geometry, CNumerics ******numerics, CConfig **config, unsigned short iZone, unsigned short iInst, unsigned short kind_recording){

  /*--- For the one-shot strategy conservative variables as well as mesh coordinates are recorded. Furthermore, we need to record the mesh coordinates in every flow iteration,
   *  thus we make use of the COMBINED recording in each step ---*/

  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();
  bool heat = config[iZone]->GetWeakly_Coupled_Heat();
  if ((kind_recording == MESH_COORDS) || (kind_recording == NONE)  || (kind_recording == COMBINED) ||
      (kind_recording == GEOMETRY_CROSS_TERM) || (kind_recording == ALL_VARIABLES)){

    /*--- Update geometry to get the influence on other geometry variables (normals, volume etc) ---*/

    geometry[iZone][iInst][MESH_0]->UpdateGeometry(geometry[iZone][iInst], config[iZone]);

  }

  /*--- Compute coupling between flow and turbulent equations ---*/

  solver[iZone][iInst][MESH_0][FLOW_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  solver[iZone][iInst][MESH_0][FLOW_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);

  if (turbulent && !frozen_visc){
    solver[iZone][iInst][MESH_0][FLOW_SOL]->Preprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, true);
    solver[iZone][iInst][MESH_0][TURB_SOL]->Postprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0);
    solver[iZone][iInst][MESH_0][TURB_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
    solver[iZone][iInst][MESH_0][TURB_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);

  }

  if (heat){
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Set_Heatflux_Areas(geometry[iZone][iInst][MESH_0], config[iZone]);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Preprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0, NO_RK_ITER, RUNTIME_HEAT_SYS, true);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->Postprocessing(geometry[iZone][iInst][MESH_0],solver[iZone][iInst][MESH_0], config[iZone], MESH_0);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->InitiateComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
    solver[iZone][iInst][MESH_0][HEAT_SOL]->CompleteComms(geometry[iZone][iInst][MESH_0], config[iZone], SOLUTION);
  }

}

void COneShotFluidIteration::InitializeAdjoint_Update(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();

  /*--- Initialize the adjoints the conservative variables ---*/

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

    solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdjoint_OutputUpdate(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);
  }

  if (((Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == ONE_SHOT_RANS)) && !frozen_visc) {
    solver[iZone][iInst][MESH_0][ADJTURB_SOL]->SetAdjoint_OutputUpdate(geometry[iZone][iInst][MESH_0],
        config[iZone]);
  }
}

void COneShotFluidIteration::InitializeAdjoint_Zero(CSolver *****solver, CGeometry ****geometry, CConfig **config, unsigned short iZone, unsigned short iInst){

  unsigned short Kind_Solver = config[iZone]->GetKind_Solver();
  bool frozen_visc = config[iZone]->GetFrozen_Visc_Disc();

  /*--- Initialize the adjoints the conservative variables ---*/

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

    solver[iZone][iInst][MESH_0][ADJFLOW_SOL]->SetAdjoint_OutputZero(geometry[iZone][iInst][MESH_0],
                                                                  config[iZone]);
  }

  if (((Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == ONE_SHOT_RANS)) && !frozen_visc) {
    solver[iZone][iInst][MESH_0][ADJTURB_SOL]->SetAdjoint_OutputZero(geometry[iZone][iInst][MESH_0],
        config[iZone]);
  }
}

void COneShotFluidIteration::Iterate_No_Residual(COutput *output,
                                        CIntegration ****integration,
                                        CGeometry ****geometry,
                                        CSolver *****solver,
                                        CNumerics ******numerics,
                                        CConfig **config,
                                        CSurfaceMovement **surface_movement,
                                        CVolumetricMovement ***volume_grid_movement,
                                        CFreeFormDefBox*** FFDBox,
                                        unsigned short val_iZone,
                                        unsigned short val_iInst) {

  unsigned long ExtIter = config[val_iZone]->GetExtIter();
  unsigned short Kind_Solver = config[val_iZone]->GetKind_Solver();
  unsigned long IntIter = 0;
  bool unsteady = config[val_iZone]->GetUnsteady_Simulation() != STEADY;
  bool frozen_visc = config[val_iZone]->GetFrozen_Visc_Disc();

  if (!unsteady)
    IntIter = ExtIter;
  else {
    IntIter = config[val_iZone]->GetIntIter();
  }

  /*--- Extract the adjoints of the conservative input variables and store them for the next iteration ---*/

  if ((Kind_Solver == DISC_ADJ_NAVIER_STOKES) || (Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == DISC_ADJ_EULER) ||
      (Kind_Solver == ONE_SHOT_EULER) || (Kind_Solver == ONE_SHOT_NAVIER_STOKES) || (Kind_Solver == ONE_SHOT_RANS)) {

    solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->ExtractAdjoint_Solution_Clean(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

    solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->ExtractAdjoint_Variables(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone]);

    /*--- Set the convergence criteria (only residual possible) ---*/

    integration[val_iZone][val_iInst][ADJFLOW_SOL]->Convergence_Monitoring(geometry[val_iZone][val_iInst][MESH_0], config[val_iZone],
                                                                          IntIter, log10(solver[val_iZone][val_iInst][MESH_0][ADJFLOW_SOL]->GetRes_RMS(0)), MESH_0);

    }
  if (((Kind_Solver == DISC_ADJ_RANS) || (Kind_Solver == ONE_SHOT_RANS)) && !frozen_visc) {

    solver[val_iZone][val_iInst][MESH_0][ADJTURB_SOL]->ExtractAdjoint_Solution_Clean(geometry[val_iZone][val_iInst][MESH_0],
                                                                              config[val_iZone]);
  }

  }
