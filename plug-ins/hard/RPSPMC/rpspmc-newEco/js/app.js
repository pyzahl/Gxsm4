(function(APP, $, undefined) {
    
    // App configuration
    APP.config = {};
    APP.config.app_id = 'rpspmc';
    APP.config.app_url = '/bazaar?start=' + APP.config.app_id + '?' + location.search.substr(1);
    APP.config.socket_url = 'ws://' + window.location.hostname + ':9002';

    //console.log ("APP URL: " + APP.config.app_url )
    //console.log ("SOCKET URL :" + APP.config.socket_url )

    // WebSocket
    APP.ws = null;

    // Plot
    APP.plot = {};

    APP.param_callbacks = {};

    APP.param_callbacks["PACPLL_RUN"] = APP.processRun;
    
    // Signal stack
    APP.signalStack = [];

    // Parameter stack
    APP.parameterStack = [];
    APP.params = {
	orig: {},
	old: {},
	local: {}
    };
    
    // Parameters
    APP.dc_tau = 100.0; // ms
    APP.pactau = 50.0; // us
    APP.pacatau = 20.0; // us
    APP.frequency = 30755.0; //32766.0;
    APP.aux_scale = 0.0116; // 20Hz/V fixed for this app
    APP.volume = 200.0; // mV
    APP.operation = 0;
    APP.TransportDecimation = 16;
    APP.TransportMode = 0;
    APP.TransportCh3 = 0;
    APP.TransportCh4 = 1;
    APP.TransportCh5 = 5;
    
    APP.pacverbose = 0;
    APP.gain1 = 1;
    APP.gain2 = 1;
    APP.gain3 = 1;
    APP.gain4 = 1;
    APP.gain5 = 1;
    APP.shr_dec = 4;

    APP.tune_timer = null;
    APP.tune_f   =  0.0;
    APP.interval = -1.0;
    APP.fspan =  10.0;
    APP.dfreq =   0.05;
    
    APP.AMPLITUDE_FB_SETPOINT = 20.0;
    APP.AMPLITUDE_FB_CP = 0.0;
    APP.AMPLITUDE_FB_CI = 0.0;
    APP.EXEC_FB_UPPER = 500.0;
    APP.EXEC_FB_LOWER = -100.0;
    APP.AMPLITUDE_CONTROL = false;

    APP.PHASE_FB_SETPOINT = 0.0;
    APP.PHASE_FB_CP = 0.0;
    APP.PHASE_FB_CI = 0.0;
    APP.FREQ_FB_UPPER = 500.0;
    APP.FREQ_FB_LOWER = 0.0;
    APP.PHASE_CONTROL = false;
    
    APP.compressed_data = 0;
    APP.decompressed_data = 0;
    // App state
    APP.state = {
        socket_opened: false,
        processing: false,
    };

    // Starts template application on server
    APP.startApp = function() {

        $.get(APP.config.app_url)
            .done(function(dresult) {
                if (dresult.status == 'OK') {
                    APP.connectWebSocket();
                } else if (dresult.status == 'ERROR') {
                    console.log(dresult.reason ? dresult.reason : 'Could not start the application (ERR1)');
                    APP.startApp();
                } else {
                    console.log('Could not start the application (ERR2)');
                    APP.startApp();
                }
            })
            .fail(function() {
                console.log('Could not start the application (ERR3)');
                APP.startApp();
            });
    };


    APP.closeWebSocket = function() {
        if (APP.ws) {
	    APP.state.socket_opened = false;
            APP.ws.close ();
	}
    };

    APP.connectWebSocket = function() {

        //Create WebSocket
        if (window.WebSocket) {
            APP.ws = new WebSocket(APP.config.socket_url);
            APP.ws.binaryType = "arraybuffer";
        } else if (window.MozWebSocket) {
            APP.ws = new MozWebSocket(APP.config.socket_url);
            APP.ws.binaryType = "arraybuffer";
        } else {
            console.log('Browser does not support WebSocket');
        }


        // Define WebSocket event listeners
        if (APP.ws) {

            APP.ws.onopen = function() {
                $('#hello_message').text("GXSM3 Red Pitaya PACPLL Webinterface");
                console.log('Socket opened');   
                APP.state.socket_opened = true;
		
		APP.params.local = {};
                setTimeout(APP.loadParams, 20);

                // Set initial parameters
		// NOTE: server is crashing/aborting if any paramert is out of range, JSON names do not match, invaling vars assigned, invalid HTML id's, etc. etc.!!!
                APP.setDCtau();
                APP.setPactau();
                APP.setPacAtau();
                APP.setFrequency();
                APP.setVolume();
                APP.setOperation();
                APP.setPACVerbose();

		APP.setTransportDecimation();
		APP.setTransportMode();
		APP.setTransportCh3();
		APP.setTransportCh4();
		APP.setTransportCh5();
                APP.setGain1();
                APP.setGain2();
                APP.setGain3();
                APP.setGain4();
                APP.setGain5();
		APP.setShrDec();

		console.log('WS open, send init params done.');
            };

            APP.ws.onclose = function() {
                APP.state.socket_opened = false;
                console.log('Socket closed');
            };

            APP.ws.onerror = function(ev) {
                $('#hello_message').text("Connection error");
                console.log('Websocket error: ', ev);         
            };

            APP.ws.onmessage = function(ev) {
                //console.log('Message recieved');

                //Capture signals
                if (APP.state.processing) {
                    return;
                }
                APP.state.processing = true;

                try {
                    var data = new Uint8Array(ev.data);
		    APP.compressed_data += data.length;
                    var inflate = pako.inflate(data);
                    var text = String.fromCharCode.apply(null, new Uint8Array(inflate));
		    APP.decompressed_data += text.length;
                    var receive = JSON.parse(text);

                    if (receive.parameters) {
			//console.log ("**ws received parameters");
			console.log ("APP.ws.onmessage" + receive.parameters);

                        APP.parameterStack.push (receive.parameters);
                        if ((Object.keys(APP.params.orig).length == 0) && (Object.keys(receive.parameters).length == 0)) {
                            APP.params.local['in_command'] = { value: 'send_all_params' };
                            APP.ws.send(JSON.stringify({ parameters: APP.params.local }));
                            APP.params.local = {};
                        } else {
                            APP.parameterStack.push(receive.parameters);
                        }
                    }

                    if (receive.signals) {
                        APP.signalStack.push(receive.signals);
                    }
                    APP.state.processing = false;
                } catch (e) {
                    APP.state.processing = false;
                    console.log(e);
                } finally {
                    APP.state.processing = false;
                }
            };
        }
    };

    // Sends to server modified parameters
    APP.sendParams = function() {
        if ($.isEmptyObject(APP.params.local)) {
            return false;
        }

        if (!APP.state.socket_opened) {
            console.log('ERROR: Cannot save changes, socket not opened');
            return false;
        }

        APP.params.local['in_command'] = { value: 'send_all_params' };
        APP.ws.send(JSON.stringify({ parameters: APP.params.local }));
        APP.params.local = {};
        return true;
    };

    APP.performanceHandler = function() {
        $('#throughput_view2').text((APP.compressed_data / 1024).toFixed(2) + "kB/s");
        $('#connection_icon').attr('src', '../assets/images/good_net.png');
        $('#connection_meter').attr('title', 'It seems like your connection is ok');
        g_PacketsRecv = 0;

	APP.compressed_data = 0;
	APP.decompressed_data = 0;
    }



    // Handler
    APP.signalHandler = function() {
        if (APP.signalStack.length > 0) {
            APP.processSignals(APP.signalStack[0]);
            APP.signalStack.splice(0, 1);
        }
        if (APP.signalStack.length > 2)
            APP.signalStack.length = [];
    }

    APP.parametersHandler = function() {
        if (APP.parameterStack.length > 0) {
            APP.processParameters (APP.parameterStack[0]);
            APP.parameterStack.splice (0, 2); 
        }
    };
    
    // Processes newly received values for parameters
    APP.processParameters = function(new_params) {
        //APP.params.old = $.extend(true, {}, APP.params.orig);
        APP.params.old = APP.params.orig;
        var old_params = APP.params.old;
        for (var param_name in new_params) {
            // Save new parameter value
            APP.params.orig[param_name] = new_params[param_name];
            console.log('APP.processParameters: ' + param_name + new_params[param_name]);
 
            if (APP.param_callbacks[param_name] !== undefined){
                APP.param_callbacks[param_name](new_params);
            }
        }     
    };
    
    APP.statusParameterShowHandler = function() {
        //Stats - parameters         $('#ch1_reading').text(parseFloat(ch1).toFixed(4));
        $('#dc_offset').text(parseFloat (1.0*APP.params.orig['DC_OFFSET'].value).toFixed(4) + "mV");
        $('#exec_amp_mon').text(parseFloat (1.0*APP.params.orig['EXEC_MONITOR'].value).toFixed(4) + "mV");
        $('#dds_freq_mon').text(parseFloat (1.0*APP.params.orig['DDS_FREQ_MONITOR'].value).toFixed(4) + "Hz");
        $('#dds_volume_mon').text(parseFloat (1.0*APP.params.orig['VOLUME_MONITOR'].value).toFixed(4) + "mV");
        $('#phase_mon').text(parseFloat (1.0*APP.params.orig['PHASE_MONITOR'].value).toFixed(4) + "deg");
        $('#info_cpu').text(floatToLocalString(shortenFloat(APP.params.orig['CPU_LOAD'].value)));
        $('#info_ram').text(floatToLocalString(shortenFloat(APP.params.orig['FREE_RAM'].value/(1024*1024))));
        $('#info_counter').text(APP.params.orig['COUNTER'].value);
        //$('#info_text').text(APP.params.orig['PAC_TEXT'].value); // ???? undefined ????
        //$('#info_period').text(APP.params.orig['PERIOD'].value);
    };

    APP.loadParams = function() {
        APP.params.local = {};
        APP.params.local['PARAMETER_PERIOD'] = { value: 200 }; //RO
        APP.params.local['SIGNAL_PERIOD'] = { value: 200 }; //RO
        APP.ws.send(JSON.stringify({ parameters: APP.params.local }));
        APP.params.local = {};
    };
       
    //Set gain
    APP.setGain1 = function() {

        APP.gain1 = $('#gain1_set').val();

        var local = {};
        local['GAIN1'] = { value: APP.gain1 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#gain1_value').text(APP.gain1);

    };

    APP.setShrDec = function() {

        APP.shr_dec = $('#shr_dec_set').val();

        var local = {};
        local['SHR_DEC_DATA'] = { value: APP.shr_dec };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#shr_dec_value').text(APP.shr_dec);

    };

    //Set gain
    APP.setGain2 = function() {

        APP.gain2 = $('#gain2_set').val();

        var local = {};
        local['GAIN2'] = { value: APP.gain2 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#gain2_value').text(APP.gain2);

    };
    

    //Set gain
    APP.setGain3 = function() {

        APP.gain3 = $('#gain3_set').val();

        var local = {};
        local['GAIN3'] = { value: APP.gain3 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#gain3_value').text(APP.gain3);

    };

    //Set gain
    APP.setGain4 = function() {

        APP.gain4 = $('#gain4_set').val();

        var local = {};
        local['GAIN4'] = { value: APP.gain4 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#gain4_value').text(APP.gain4);

    };

    //Set gain
    APP.setGain5 = function() {

        APP.gain5 = $('#gain5_set').val();

        var local = {};
        local['GAIN5'] = { value: APP.gain5 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#gain5_value').text(APP.gain5);

    };

    // Set DC tau
    APP.setDCtau = function() {

        APP.dc_tau = $('#dctau_set').val();

        var local = {};
        local['PAC_DCTAU'] = { value: APP.dc_tau };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#dctau_value').text(APP.dc_tau);

    };

    // Set PAC tau (phase LMS path)
    
    APP.setPactau = function() {

        APP.pactau = $('#pactau_set').val();

        var local = {};
        local['PACTAU'] = { value: APP.pactau };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#pactau_value').text(APP.pactau);

    };

    // Set pacatau (ampl LMS path)
    APP.setPacAtau = function() {

        APP.apactau = $('#pacatau_set').val();

        var local = {};
        local['PACATAU'] = { value: APP.pacatau };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#pacatau_value').text(APP.pacatau);

    };

    // Set frequency
    APP.setFrequency = function() {

        APP.frequency = $('#frequency_set').val();

        var local = {};
        local['FREQUENCY_MANUAL'] = { value: APP.frequency };
        APP.ws.send(JSON.stringify({ parameters: local }));

	// use same
        local['FREQUENCY_CENTER'] = { value: APP.frequency };
        APP.ws.send(JSON.stringify({ parameters: local }));

	// set aux scale
        local['AUX_SCALE'] = { value: APP.aux_scale };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#frequency_value').text(APP.frequency);

    };

    // Set volume
    APP.setVolume = function() {

        APP.volume = $('#volume_set').val();
	var volt = APP.volume;
	
        var local = {};
        local['VOLUME_MANUAL'] = { value: volt };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#volume_value').text(APP.volume);

    };

    // Set operation
    APP.setOperation = function() {

        APP.operation = $('#operation_set').val();

        console.log('Set OP to ' + APP.operation);

        var local = {};
        local['OPERATION'] = { value: APP.operation };
        APP.ws.send(JSON.stringify({ parameters: local }));
    };

    // Set pacverbose
    APP.setPACVerbose = function() {

        APP.pacverbose = $('#pacverbose_set').val();

        var local = {};
        local['PACVERBOSE'] = { value: APP.pacverbose };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#pacverbose_value').text(APP.pacverbose);
    };

    APP.setTransportDecimation = function() {

        APP.TransportDecimation = $('#decimation_set').val();

        var local = {};
        local['TRANSPORT_DECIMATION'] = { value: APP.TransportDecimation };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#decimation_value').text(APP.TransportDecimation);
    };
    
    APP.setTransportMode = function() {


        APP.transport_mode = $('#transport_mode_set').val();

        var local = {};
        local['TRANSPORT_MODE'] = { value: APP.transport_mode };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#transport_mode_value').text(APP.transport_mode);
    };
    
    APP.setTransportCh3 = function() {
	
        APP.transport_ch3 = $('#transport_ch3_set').val();

        var local = {};
        local['TRANSPORT_CH3'] = { value: APP.transport_ch3 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#transport_ch3_value').text(APP.transport_ch3);
    };
    
    APP.setTransportCh4 = function() {

        APP.transport_ch4 = $('#transport_ch4_set').val();

        var local = {};
        local['TRANSPORT_CH4'] = { value: APP.transport_ch4 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#transport_ch4_value').text(APP.transport_ch4);
    };

    APP.setTransportCh5 = function() {

        APP.transport_ch5 = $('#transport_ch5_set').val();

        var local = {};
        local['TRANSPORT_CH5'] = { value: APP.transport_ch5 };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#transport_ch5_value').text(APP.transport_ch5);
    };

    
    APP.setTunedf = function() {
	APP.dfreq = $('#df_set').val();
        var local = {};
        local['TUNE_DFREQ'] = { value: APP.dfreq };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#df_value').text(APP.dfreq);
    };
    
    APP.setTunefs = function() {
	APP.fspan = $('#frequency_span_set').val();
	APP.tune_f = APP.frequency-APP.fspan/2;

        var local = {};
        local['TUNE_SPAN'] = { value: APP.fspan };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#frequency_span_value').text(APP.fspan);
    };
    
    // Tune in next freq
    APP.tuneNext = function() {
	APP.tune_f += 1.0*APP.dfreq;
	
	if (APP.tune_f > 1.0*APP.frequency+APP.fspan/2){
	    APP.dfreq = -Math.abs (1.0*APP.dfreq);
	    APP.tune_f = 1.0*APP.frequency + 1.0*APP.fspan/2;
            $('#df_value').text(APP.dfreq);
	}
	if (APP.tune_f < 1.0*APP.frequency-APP.fspan/2){
	    APP.dfreq = Math.abs (1.0*APP.dfreq);
	    APP.tune_f = 1.0*APP.frequency - 1.0*APP.fspan/2;
            $('#df_value').text(APP.dfreq);
	}
	
        var local = {};
        local['FREQUENCY_MANUAL'] = { value: APP.tune_f };
        APP.ws.send(JSON.stringify({ parameters: local }));
	
        $('#frequency_value').text(APP.tune_f);
    };
    



    // Set Ampl Controller
    APP.setAMPLITUDE_FB_SETPOINT = function() {

        APP.AMPLITUDE_FB_SETPOINT = $('#ampl_setpoint_set').val(); // mV

        var local = {};
        local['AMPLITUDE_FB_SETPOINT'] = { value: APP.AMPLITUDE_FB_SETPOINT };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#ampl_setpoint_value').text(APP.AMPLITUDE_FB_SETPOINT);

    };

    APP.setAMPLITUDE_FB_CP = function() {

        //write_pll_variable32 (dsp_pll.icoef_Amp, pll.signum_ci_Amp * CPN(29)*pow (10.,pll.ci_gain_Amp/20.));
        // = ISign * CPN(29)*pow(10.,Igain/20.);
		
        //write_pll_variable32 (dsp_pll.pcoef_Amp, pll.signum_cp_Amp * CPN(29)*pow (10.,pll.cp_gain_Amp/20.));
        // = PSign * CPN(29)*pow(10.,Pgain/20.);

        APP.AMPLITUDE_FB_CP = ($('#ampl_cp_gain_inv').is(':checked')?-1:1) * Math.pow(10.,$('#ampl_cp_gain_set').val()/20);
        console.log('AMPL FB CP = ' + $('#ampl_cp_gain_set') + ' => ' + APP.AMPLITUDE_FB_CP);

        var local = {};
        local['AMPLITUDE_FB_CP'] = { value: APP.AMPLITUDE_FB_CP };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#ampl_cp_gain_value').text(APP.AMPLITUDE_FB_CP);

    };

    APP.setAMPLITUDE_FB_CI = function() {

        APP.AMPLITUDE_FB_CI = ($('#ampl_ci_gain_inv').is(':checked')?-1:1) * Math.pow(10.,$('#ampl_ci_gain_set').val()/20);

        var local = {};
        local['AMPLITUDE_FB_CI'] = { value: APP.AMPLITUDE_FB_CI };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#ampl_ci_gain_value').text(APP.AMPLITUDE_FB_CI);

    };

    APP.setEXEC_FB_UPPER = function() {

        APP.EXEC_FB_UPPER = $('#exec_upper_set').val();

        var local = {};
        local['EXEC_FB_UPPER'] = { value: APP.EXEC_FB_UPPER };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#exec_upper_value').text(APP.EXEC_FB_UPPER);

    };

    APP.setEXEC_FB_LOWER = function() {

        APP.EXEC_FB_LOWER = $('#exec_lower_set').val();

        var local = {};
        local['EXEC_FB_LOWER'] = { value: APP.EXEC_FB_LOWER };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#exec_lower_value').text(APP.EXEC_FB_LOWER);

    };

    APP.setAmplLoopControl = function() {
	APP.AMPLITUDE_CONTROL = $("#ampl_control_on").is(':checked');
        console.log('Amplitude Loop Control: ', APP.AMPLITUDE_CONTROL);
        var local = {};
        local['AMPLITUDE_CONTROLLER'] = { value: APP.AMPLITUDE_CONTROL };
        APP.ws.send(JSON.stringify({ parameters: local }));
    };
    
    // Set Phase Controller 
    APP.setPHASE_FB_SETPOINT = function() {

        APP.PHASE_FB_SETPOINT = $('#phase_setpoint_set').val();

        var local = {};
        local['PHASE_FB_SETPOINT'] = { value: APP.PHASE_FB_SETPOINT };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#phase_setpoint_value').text(APP.PHASE_FB_SETPOINT);

    };

    APP.setPHASE_FB_CP = function() {

        APP.PHASE_FB_CP = ($('#phase_cp_gain_inv').is(':checked')?-1:1) * Math.pow(10.,$('#phase_cp_gain_set').val()/20);

        var local = {};
        local['PHASE_FB_CP'] = { value: APP.PHASE_FB_CP };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#phase_cp_gain_value').text(APP.PHASE_FB_CP);

    };

    APP.setPHASE_FB_CI = function() {

        APP.PHASE_FB_CI = ($('#phase_ci_gain_inv').is(':checked') ? -1:1) * Math.pow(10.,$('#phase_ci_gain_set').val()/20);

        var local = {};
        local['PHASE_FB_CI'] = { value: APP.PHASE_FB_CI };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#phase_ci_gain_value').text(APP.PHASE_FB_CI);

    };

    APP.setFREQ_FB_UPPER = function() {

        APP.FREQ_FB_UPPER = $('#exec_upper_set').val();

        var local = {};
        local['FREQ_FB_UPPER'] = { value: APP.FREQ_FB_UPPER };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#exec_upper_value').text(APP.FREQ_FB_UPPER);

    };

    APP.setFREQ_FB_LOWER = function() {

        APP.FREQ_FB_LOWER = $('#exec_lower_set').val();

        var local = {};
        local['FREQ_FB_LOWER'] = { value: APP.FREQ_FB_LOWER };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#exec_lower_value').text(APP.FREQ_FB_LOWER);

    };

    APP.setPhaseLoopControl = function() {
	APP.PHASE_CONTROL = $("#phase_control_on").is(':checked');
        console.log('Phase Loop Control: ', APP.PHASE_CONTROL);
        var local = {};
        local['PHASE_CONTROLLER'] = { value: APP.PHASE_CONTROL };
        APP.ws.send(JSON.stringify({ parameters: local }));
    };
    

    /*
    APP.set@ = function() {

        APP.@ = $('#@_set').val();

        var local = {};
        local['@'] = { value: APP.@ };
        APP.ws.send(JSON.stringify({ parameters: local }));

        $('#@_value').text(APP.@);

    };
    */







    
    //APP.tuneNext = function() { };
    
    // Setup Tune Timer
    APP.setTuneTimer = function() {

	if (APP.interval > 10.0)
	    clearInterval (APP.tune_timer);
	
	APP.interval = $('#interval_set').val();
	if (APP.interval > 10.0 && APP.interval < 10000.0){
	    APP.tune_f = APP.frequency-APP.fspan/2;
	    APP.tune_timer = setInterval (APP.tuneNext, APP.interval);
	    $('#interval_value').text(APP.interval);
	} else {
	    $('#interval_value').text(0);
	}
    };

    // Processes newly received data for signals
    APP.processSignals = function(new_signals) {

        var labelArr = [];
        var pointArr = [];
        var ch1=0.;
	var ch2=0.;
	var ch3=0.;
	var ch4=0.;
	var ch5=0.;

	//var plot = $.plot("#placeholder", [
	//		{ data: sin, label: "sin(x)"},
	//		{ data: cos, label: "cos(x)"}
	//	], {
	
        //console.log('APP.processSIgnals');

	var xmode=0;
	
        // Draw signals
        for (sig_name in new_signals) {
	    if (sig_name == 'SIGNAL_FRQ'){
		//console.log('APP.processSignals Df-tune=%g',  APP.params.orig['FREQUENCY_TUNE'].value);
		xmode=1;
		break;
	    }
	}
	xmode=0; // override
	
        // Draw signals
        for (sig_name in new_signals) {

            // Ignore empty signals
            if (new_signals[sig_name].size == 0) continue;

            var points = [];
	    if (xmode == 1)
		for (var i = 0; i < new_signals[sig_name].size; i++) {
                    points.push([new_signals['SIGNAL_FRQ'].value[i], new_signals[sig_name].value[i]]);
		}
	    else
		for (var i = 0; i < new_signals[sig_name].size; i++) {
                    points.push([i, new_signals[sig_name].value[i]]);
		}

            pointArr.push(points);
            labelArr.push(sig_name);

	    if (sig_name == 'SIGNAL_CH1')
		ch1 = new_signals[sig_name].value[new_signals[sig_name].size - 2];
	    if (sig_name == 'SIGNAL_CH2')
		ch2 = new_signals[sig_name].value[new_signals[sig_name].size - 2];
	    if (sig_name == 'SIGNAL_CH3')
		ch3 = new_signals[sig_name].value[new_signals[sig_name].size - 1];
	    if (sig_name == 'SIGNAL_CH4')
		ch4 = new_signals[sig_name].value[new_signals[sig_name].size - 1];
	    if (sig_name == 'SIGNAL_CH5')
		ch5 = new_signals[sig_name].value[new_signals[sig_name].size - 1];
	    if (sig_name == 'SIGNAL_FRQ')
		frq = new_signals[sig_name].value[new_signals[sig_name].size - 1];

        }

	//var plot_data = { label: labelArr, data: pointArr };
	var plot_data = [
	    { label: labelArr[0], data: pointArr[0] },
	    { label: labelArr[1], data: pointArr[1] },
	    { label: labelArr[2], data: pointArr[2] },
	    { label: labelArr[3], data: pointArr[3] },
	    { label: labelArr[4], data: pointArr[4] },
	    { label: labelArr[5], data: pointArr[5] }
	];

        //Update value
        $('#ch1_reading').text(parseFloat(ch1).toFixed(4));
        $('#ch2_reading').text(parseFloat(ch2).toFixed(4));
        $('#ch3_reading').text(parseFloat(ch3).toFixed(4));
        $('#ch4_reading').text(parseFloat(ch4).toFixed(4));
        $('#ch5_reading').text(parseFloat(ch5).toFixed(4));
	var f_actual = 1.0*APP.frequency + 1.0*APP.params.orig['FREQUENCY_TUNE'].value;
	$('#df_tune_reading').text(parseFloat(APP.params.orig['FREQUENCY_TUNE'].value).toFixed(4)+' Hz ['
				   + parseFloat(f_actual).toFixed(4) + ' Hz]'
				  );

        //console.log('Flot plot update with:');
        //console.log(plot_data);
        // Update graph
        //APP.plot.setData(pointArr);

	APP.plot.setData(plot_data);
        APP.plot.resize();
        APP.plot.setupGrid();
        APP.plot.draw();
    };

    APP.SaveGraphsPNG = function() {
        html2canvas($('body'), {
            background: '#343433', // Like background of BODY
            onrendered: function(canvas) {
                var a = document.createElement('a');
                // toDataURL defaults to png, so we need to request a jpeg, then convert for file download.
                a.href = canvas.toDataURL("image/jpeg").replace("image/jpeg", "image/octet-stream");
                a.download = 'graphs.jpg';
                // a.click(); // Not working with firefox
                fireEvent(a, 'click');
            }
        });
    }

    APP.processRun = function(new_params) {
        if (new_params['PACPLL_RUN'].value === true) {
            $('#PACPLL_RUN').hide();
            $('#PACPLL_STOP').css('display', 'block');
        } else {
            $('#PACPLL_STOP').hide();
            $('#PACPLL_RUN').show();
        }
    }



    setInterval(APP.signalHandler, 20);
    setInterval(APP.parametersHandler, 200);

    setInterval(APP.performanceHandler, 1000);
    setInterval(APP.statusParameterShowHandler, 333);

}(window.APP = window.APP || {}, jQuery));



function getLocalDecimalSeparator() {
    var n = 1.1;
    return n.toLocaleString().substring(1,2);
}

function parseLocalFloat(num) {
    return +(num.replace(getLocalDecimalSeparator(), '.'));
}

function floatToLocalString(num) {
    // Workaround for a bug in Safari 6 (reference: https://github.com/mleibman/SlickGrid/pull/472)
    //return num.toString().replace('.', getLocalDecimalSeparator());
    return (num + '').replace('.', getLocalDecimalSeparator());
}

function shortenFloat(value) {
    return value.toFixed(Math.abs(value) >= 10 ? 1 : 2);
}

// Page onload event handler
$(function() {

    //Init plot
    APP.plot = $.plot($("#placeholder"), [], { 
        series: {
            shadowSize: 0, // Drawing is faster without shadows
        },
        yaxis: {
            min: -1000,
            max: 1000
        },
        xaxis: {
            min: 0,
            max: 1024,
            show: true //false
        },
	legend: {
	    show: true,     
	    position: "ne",     
	    //backgroundColor: "#fdff00",
	    backgroundOpacity: "0",
	    margin: [-50,0]
	    //container: null or jQuery object/DOM element/jQuery expression
	}
	//colors: [ '#20f', '#00ff4b', '#f00', '#fdff00']
    });

    $( "#pac_menu" ).on("click", function() {
	$('#plotter_control').hide();
	$('#ampl_control').hide();
	$('#phase_control').hide();
	$('#pac_control').show(); 
    });

    $( "#dbg_menu" ).on("click", function() {
	$('#dbg_info').show();
    });

    $( "#ampl_menu" ).on("click", function() {
	$('#plotter_control').hide();
	$('#dbg_info').hide();
	$('#phase_control').hide();
	$('#pac_control').hide();
	$('#ampl_control').show(); 
    });

    $( "#phase_menu" ).on("click", function() {
        console.log('Phase Menu Show');
	$('#plotter_control').hide();
	$('#dbg_info').hide();
	$('#ampl_control').hide();
	$('#pac_control').hide();
	$('#phase_control').show(); 
    });

    $( "#plotter_menu" ).on("click", function() {  
        console.log('Plotter Menu Show');
	$('#pac_control').hide();
	$('#dbg_info').hide();
	$('#phase_control').hide();
	$('#ampl_control').hide(); 
	$('#plotter_control').show();
    });

    // Input change
    $("#interval_set").on("change input", function() {

	APP.setTuneTimer ();
    });
    
    $("#df_set").on("change input", function() {

	APP.setTunedf ();
    });
    
    $("#frequency_span_set").on("change input", function() {

	APP.setTunefs ();
    });
    
    // Input change
    $("#gain1_set").on("change input", function() {

        APP.setGain1(); 
    });

    $("#shr_dec_set").on("change input", function() {

	APP.setShrDec();
    });

    
    // Input change
    $("#gain2_set").on("change input", function() {

        APP.setGain2(); 
    });
    
    // Input change
    $("#gain3_set").on("change input", function() {

        APP.setGain3(); 
    });
    
    // Input change
    $("#gain4_set").on("change input", function() {

        APP.setGain4(); 
    });

    // Input change
    $("#gain5_set").on("change input", function() {

        APP.setGain5(); 
    });

    // Input change
    $("#dctau_set").on("change input", function() {

        APP.setDCtau(); 
    });

    // Input change
    $("#pactau_set").on("change input", function() {

        APP.setPactau(); 
    });

    // Input change
    $("#pacatau_set").on("change input", function() {

        APP.setPacAtau(); 
    });

    // Input change
    $("#frequency_set").on("change input", function() {

        APP.setFrequency(); 
    });

    $("#volume_set").on("change input", function() {

        APP.setVolume(); 
    });

    $("#operation_set").on("change", function() {
        console.log('Operation Mode Changed');
        APP.setOperation(); 
    });

    // AMPL CONTROL
    $("#ampl_setpoint_set").on("change", function() {

        APP.setAMPLITUDE_FB_SETPOINT(); 
    });

    $("#ampl_cp_gain_set").on("change", function() {

        APP.setAMPLITUDE_FB_CP(); 
    });

    $("#ampl_ci_gain_set").on("change", function() {

        APP.setAMPLITUDE_FB_CI(); 
    });

    $("#exec_upper_set").on("change", function() {

        APP.setEXEC_FB_UPPER(); 
    });

    $("#exec_lower_set").on("change", function() {

        APP.setEXEC_FB_LOWER(); 
    });

    $("#ampl_control_on").on('change', function() {
        APP.setAmplLoopControl(); 
    });

    // PHASE CONTROL
    $("#phase_setpoint_set").on("change", function() {

        APP.setPHASE_FB_SETPOINT(); 
    });

    $("#phase_cp_gain_set").on("change", function() {

        APP.setPHASE_FB_CP(); 
    });

    $("#phase_ci_gain_set").on("change", function() {

        APP.setPHASE_FB_CI(); 
    });

    $("#freq_upper_set").on("change", function() {

        APP.setFREQ_FB_UPPER(); 
    });

    $("#freq_lower_set").on("change", function() {

        APP.setFREQ_FB_LOWER(); 
    });

    
    $("#phase_control_on").on('change', function() {
        APP.setPhaseLoopControl(); 
    });
    
    
    $("#pacverbose_set").on("change input", function() {

        APP.setPACVerbose(); 
    });

    $("#decimation_set").on("change", function() {

        APP.setTransportDecimation(); 
    });

    $("#transport_mode_set").on("change", function() {

        APP.setTransportMode(); 
    });

    $("#transport_ch3_set").on("change", function() {

        APP.setTransportCh3(); 
    });

    $("#transport_ch4_set").on("change", function() {

        APP.setTransportCh4(); 
    });

    $("#transport_ch5_set").on("change", function() {

        APP.setTransportCh5(); 
    });


    // Process clicks on top menu buttons
    $('#PACPLL_RUN').on('click', function(ev) {
        ev.preventDefault();
        $('#PACPLL_RUN').hide();
        $('#PACPLL_STOP').css('display', 'block');
        APP.params.local['PACPLL_RUN'] = { value: true };
        APP.sendParams();
        APP.running = true;
	APP.startApp();
	console.log('PACPLL: RUN, Restart App and reopen WS');
    });

    $('#PACPLL_STOP').on('click', function(ev) {
        ev.preventDefault();
        $('#PACPLL_STOP').hide();
        $('#PACPLL_RUN').show();
        APP.params.local['PACPLL_RUN'] = { value: false };
        APP.sendParams();
        APP.running = false;
	APP.closeWebSocket();
	console.log('PACPLL: STOP, close WS.');
    });

    $('#PACPLL_SAVEGRAPH').on('click', function() {
        setTimeout(APP.SaveGraphsPNG, 30);
    });

    $('#PACPLL_SAVECSV').on('click', function() {
        $(this).attr('href', APP.downloadDataAsCSV("pacpllData.csv"));
    });


    
    // Start application
    APP.startApp();
});
