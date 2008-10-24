/*
 *  MatroskaExportDispatch.h
 *
 *    MatroskaExportDispatch.h - ComponentDispatchHelper.c required macros for export.
 *
 *
 *  Copyright (c) 2006  David Conrad
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; 
 *  version 2.1 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#if __MACH__
	ComponentSelectorOffset (-kQTRemoveComponentPropertyListenerSelect)
#else
	ComponentSelectorOffset (-kComponentVersionSelect)
#endif

	ComponentRangeCount (3)
	ComponentRangeShift (7)
	ComponentRangeMask	(7F)

	 ComponentRangeBegin (0)
#if __MACH__
		ComponentError (RemoveComponentPropertyListener)
		ComponentError (AddComponentPropertyListener)
		StdComponentCall (SetComponentProperty)
		StdComponentCall (GetComponentProperty)
		StdComponentCall (GetComponentPropertyInfo)

		ComponentError   (GetPublicResource)
		ComponentError   (ExecuteWiredAction)
		ComponentError   (GetMPWorkFunction)
		ComponentError   (Unregister)
        
		ComponentError   (Target)
		ComponentError   (Register)
#endif
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)

	ComponentRangeUnused (1)

	ComponentRangeBegin (2)
		ComponentError (ToHandle)
		ComponentCall  (ToFile)
		ComponentError (130)
		ComponentError (GetAuxiliaryData)
		ComponentCall  (SetProgressProc)
		ComponentError (SetSampleDescription)
		ComponentError  (DoUserDialog)
		ComponentError (GetCreatorType)
		ComponentCall  (ToDataRef)
		ComponentCall  (FromProceduresToDataRef)
		ComponentCall  (AddDataSource)
		ComponentCall  (Validate)
		ComponentError  (GetSettingsAsAtomContainer)
		ComponentError  (SetSettingsFromAtomContainer)
		ComponentCall  (GetFileNameExtension)
		ComponentCall  (GetShortFileTypeString)
		ComponentCall  (GetSourceMediaType)
		ComponentError (SetGetMoviePropertyProc)
	ComponentRangeEnd (2)
	
	ComponentRangeBegin (3)
		ComponentError (NewGetDataAndPropertiesProcs)
		ComponentError (DisposeGetDataAndPropertiesProcs)
	ComponentRangeEnd (3)