% Converts the text file output of the C model into data that matlab can work with and saves as a .mat file

function txt2mat(p)

% INPUT STRUCTURE
% ________________________________________________________________________________________
% Mandatory inputs 
% p.datatype: give string of type of file to convert - 'TEXT' or 'SPIKES'
% p.filename: give string of filename to convert, e.g. '1.txt'
% ________________________________________________________________________________________
% Mandatory inputs for SPIKE only
% p.gridlength: specify size of the grid, along one dimension as integer, e.g. for a 30x30 grid you would use 30
% p.timelength: number of time steps as integer 
% p.timestep: time step of simulation as decimal 
% ________________________________________________________________________________________
% Optional inputs
% p.filenameI: give string of second filename to convert - for combining values of excitatory/inhibitory spikes or voltages. p.filenameI should apply to inhibitory neurons in this case, whilst p.filename should apply to excitatory neurons 

% OUTPUT
% Saves .mat file with same name as input structure(s), e.g. 0.mat for 0.txt and 01.mat for 0.txt and 1.txt. The output for is spikeCell for SPIKES and varGrid for TEXT


% VALIDATE INPUTS AND ASSIGN DEFAULT VALUES
if ~isfield(p,'datatype')
	error('Your input structure must have a "datatype" field!')
else
	if ~strcmp(p.datatype,'TEXT') && ~strcmp(p.datatype,'SPIKES')
		error('datatype field must be either SPIKES or TEXT!')
	end
end
if ~isfield(p,'filename')
	error('Your input structure must have a "filename" field!')
else
	if ~isstr(p.filename)
		error('Filename field must be a string!')
	end
end
if ~isfield(p,'filenameI')
	p.filenameI = [];
	warning('Entering in only one filename may result in empty values associated with excitatory/inhibitory neurons if you are looking at spikes or voltages')
end
if ~isfield(p,'gridlength') && strcmp(p.datatype,'SPIKES')
	error('Input structure must have a gridlength for datatype SPIKES!')
end
if ~isfield(p,'timelength') && strcmp(p.datatype,'SPIKES')
	error('Input structure must have a timelength field for datatype SPIKES!')
end
if ~isfield(p,'timestep') && strcmp(p.datatype,'SPIKES')
	error('Input structure must have a timestep field for datatype SPIKES!')
end

% CONVERT TXT FILES TO MATLAB 
switch p.datatype

	% VARIABLES: Voltage, excitatory conductance or inhibitory conductance
	case 'TEXT' 

		% Get voltage
		try
			varGrid = csvread(sprintf('%s.txt',p.filename)); 
		catch 
			error('Error in csvread; most likely you are trying to apply TEXT method to SPIKES data')
		end
		varGrid = varGrid(:,1:end-1)';

		% Get gridlength (length along each side; assumes a square grid)
		p.gridlength = size(varGrid,1);

		% Number of time steps
		p.timelength = size(varGrid,2)/p.gridlength;
		if mod(p.timelength,1)~=0
			error('Grid does not appear to be square; code only works for square grid')
		end

		% Reshape into matrix of (gridlength x gridlength x timelength)
		varGrid = reshape(varGrid,[p.gridlength^2,p.timelength]);

		% Only for combining excitatory and inhibitory voltage values in grid with 3:1 proportionality
		if ~isempty(p.filenameI)

			% Second voltage file (inhibitory neurons)
			varGridI = csvread(sprintf('%s.txt',p.filenameI)); 
			varGridI = varGridI(:,1:end-1)';
			varGridI = reshape(varGridI,[p.gridlength^2,size(varGridI,2)/p.gridlength]);

			% Check outputs have the same gridsize
			if p.gridlength~=sqrt(size(varGridI,1))
				error('Grids of VE and VI are not the same size')
			end

			% Check outputs have the same timelength
			if p.timelength~=size(varGridI,2)
				error('VE and VI do not have the same lengths of time')
			end

			% Get odd gridpoints and use as inhibitory values (replaces dummy values in excitatory neurons)
			[x,y] = meshgrid(1:p.gridlength,1:p.gridlength);
			isinh = (mod(x,2)~=0 & mod(y,2)~=0);
			isinh = isinh(:);
			varGrid(isinh,:) = varGridI(isinh,:);

		end

		% Save as a .mat file in results folder with the same name
		save([p.filename,p.filenameI],'varGrid','-v7.3')


	case 'SPIKES'

		strvals = importdata(sprintf('%s.txt',p.filename),'\n');

		% Padding
		for i=length(strvals)+1:p.timelength
			strvals{i} = '';
		end
		coords = cellfun(@(x) sscanf(x,'%d,%d:')'+1,strvals,'UniformOutput',0);  % For horizontal list
		x=cellfun(@(x) x(1:2:end),coords,'UniformOutput',0);
		y=cellfun(@(x) x(2:2:end),coords,'UniformOutput',0);
		try
			timeCell = cellfun(@(x) sub2ind([p.gridlength,p.gridlength],x(1:2:end),x(2:2:end)),coords,'UniformOutput',0);
		catch 
			error('Error in sub2ind; most likely you are trying to apply SPIKES method to TEXT data')
		end
		spikeCell = cell(p.gridlength^2,1);

		if ~isempty(p.filenameI)

			strvalsI = importdata(sprintf('%s.txt',p.filenameI),'\n');
			% Padding
			for i=length(strvalsI)+1:p.timelength
				strvalsI{i} = '';
			end
			coordsI = cellfun(@(x) sscanf(x,'%d,%d:')'+1,strvalsI,'UniformOutput',0);  % For horizontal list
			x2=cellfun(@(x) x(1:2:end),coordsI,'UniformOutput',0);
			y2=cellfun(@(x) x(2:2:end),coordsI,'UniformOutput',0);
			timeCellI = cellfun(@(x) sub2ind([p.gridlength,p.gridlength],x(1:2:end),x(2:2:end)),coordsI,'UniformOutput',0);

			% Make spike cell
			t = 0;
			for i=1:length(timeCell)
				t=t+p.timestep;
				currentSpikes = [timeCell{i},timeCellI{i}];
				for j=currentSpikes
			        spikeCell{j} = [spikeCell{j},t];
			    end
			end

		else

			% Make spike cell
			t = 0;
			for i=1:length(timeCell)
				t=t+p.timestep;
				currentSpikes = [timeCell{i}];
				for j=currentSpikes
			        spikeCell{j} = [spikeCell{j},t];
			    end
			end
		end

	% Save as a .mat file in results folder with the same name
	save([p.filename,p.filenameI],'spikeCell')

	end

end
