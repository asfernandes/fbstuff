{
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is Adriano dos Santos Fernandes.
 * Portions created by the Initial Developer are Copyright (C) 2015 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
}

unit UdrGenRows;

interface

uses FbApi;

{
create procedure gen_rows_pascal (
    start_n integer not null,
    end_n integer not null
) returns (
    result integer not null
)
    external name 'pascaludr!gen_rows'
    engine udr;
}

type
	GenRowsInMessage = record
		start: Integer;
		startNull: WordBool;
		end_: Integer;
		endNull: WordBool;
	end;

	GenRowsInMessagePtr = ^GenRowsInMessage;

	GenRowsOutMessage = record
		result: Integer;
		resultNull: WordBool;
	end;

	GenRowsOutMessagePtr = ^GenRowsOutMessage;

	GenRowsResultSet = class(ExternalResultSetImpl)
		procedure dispose(); override;
		function fetch(status: Status): Boolean; override;

	public
		inMessage: GenRowsInMessagePtr;
		outMessage: GenRowsOutMessagePtr;
	end;

	GenRowsProcedure = class(ExternalProcedureImpl)
		procedure dispose(); override;

		procedure getCharSet(status: Status; context: ExternalContext; name: PAnsiChar;
			nameSize: Cardinal); override;

		function open(status: Status; context: ExternalContext; inMsg: Pointer;
			outMsg: Pointer): ExternalResultSet; override;
	end;

	GenRowsFactory = class(UdrProcedureFactoryImpl)
		procedure dispose(); override;

		procedure setup(status: Status; context: ExternalContext; metadata: RoutineMetadata;
			inBuilder: MetadataBuilder; outBuilder: MetadataBuilder); override;

		function newItem(status: Status; context: ExternalContext;
			metadata: RoutineMetadata): ExternalProcedure; override;
	end;

implementation

procedure GenRowsResultSet.dispose();
begin
	destroy;
end;

function GenRowsResultSet.fetch(status: Status): Boolean;
begin
	if (outMessage.result >= inMessage.end_) then
		Result := false
	else
	begin
		outMessage.result := outMessage.result + 1;
		Result := true;
	end;
end;


procedure GenRowsProcedure.dispose();
begin
	destroy;
end;

procedure GenRowsProcedure.getCharSet(status: Status; context: ExternalContext; name: PAnsiChar;
	nameSize: Cardinal);
begin
end;

function GenRowsProcedure.open(status: Status; context: ExternalContext; inMsg: Pointer;
	outMsg: Pointer): ExternalResultSet;
var
	Ret: GenRowsResultSet;
begin
	Ret := GenRowsResultSet.create();
	Ret.inMessage := inMsg;
	Ret.outMessage := outMsg;

	Ret.outMessage.resultNull := false;
	Ret.outMessage.result := Ret.inMessage.start - 1;

	Result := Ret;
end;


procedure GenRowsFactory.dispose();
begin
	destroy;
end;

procedure GenRowsFactory.setup(status: Status; context: ExternalContext; metadata: RoutineMetadata;
	inBuilder: MetadataBuilder; outBuilder: MetadataBuilder);
begin
end;

function GenRowsFactory.newItem(status: Status; context: ExternalContext;
	metadata: RoutineMetadata): ExternalProcedure;
begin
	Result := GenRowsProcedure.create;
end;


end.
