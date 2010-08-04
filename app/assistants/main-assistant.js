function MainAssistant()
{
	// subtitle random list
	this.randomSub = 
	[
		{weight: 1, text: $L('This sub-title is not random.')}
	];
	
	// setup list model
	this.mainModel = {items:[]};
	
	// setup search model
	this.searchModel = { value: '' };
	this.searchText = '';
	
	// setup menu
	this.menuModel =
	{
		visible: true,
		items:
		[
			{
				label: $L("Preferences"),
				command: 'do-prefs'
			},
			{
				label: $L("Help"),
				command: 'do-help'
			}
		]
	};
	
	this.isVisible = false;
};

MainAssistant.prototype.setup = function()
{
	// set theme because this can be the first scene pushed
	this.controller.document.body.className = prefs.get().theme;
	
	// get elements
	this.iconElement =		this.controller.get('icon');
	this.titleElement =		this.controller.get('main-title');
	this.versionElement =	this.controller.get('version');
	this.subTitleElement =	this.controller.get('subTitle');
	
	// set version string random subtitle
	this.titleElement.innerHTML = Mojo.Controller.appInfo.title;
	this.versionElement.innerHTML = "v" + Mojo.Controller.appInfo.version;
	this.subTitleElement.innerHTML = this.getRandomSubTitle();
	
	// setup menu
	this.controller.setupWidget(Mojo.Menu.appMenu, { omitDefaultItems: true }, this.menuModel);
	
	// setup buttons
        this.controller.setupWidget('drop_caches', this.attributes = { type: Mojo.Widget.activityButton },
                           this.model =
                           {
                                   buttonLabel: 'Drop Caches',
                                   buttonClass: 'palm-button',
                                   disabled: false
                           }
        );

        this.controller.setupWidget('email_cleanup', this.attributes = { type: Mojo.Widget.activityButton },
                           this.model =
                           {
                                   buttonLabel: 'Email Cleanup',
                                   buttonClass: 'palm-button',
                                   disabled: false
                           }
        );

        this.controller.setupWidget('sqlite_vaccum', this.attributes = { type: Mojo.Widget.activityButton },
                           this.model =
                           {
                                   buttonLabel: 'SQLite Vaccum',
                                   buttonClass: 'palm-button',
                                   disabled: false
                           }
        );
	
	this.visible = this.visible.bindAsEventListener(this);
	this.invisible = this.invisible.bindAsEventListener(this);
	this.controller.listen(this.controller.stageController.document, Mojo.Event.stageActivate,   this.visible);
	this.controller.listen(this.controller.stageController.document, Mojo.Event.stageDeactivate, this.invisible);
	
        this.controller.listen('drop_caches', Mojo.Event.tap, this.doAction.bindAsEventListener(this,'drop_caches'));
        this.controller.listen('email_cleanup', Mojo.Event.tap, this.doAction.bindAsEventListener(this,'email_cleanup'));
        this.controller.listen('sqlite_vaccum', Mojo.Event.tap, this.doAction.bindAsEventListener(this,'sqlite_vaccum'));
                 
        var r = new Mojo.Service.Request('palm://org.webosinternals.syshealth',
                           {
                                     method: 'status',
                                     onSuccess: this.callbackFunction.bindAsEventListener(this),
                                     onFailure: this.callbackFunction.bindAsEventListener(this)
                           }
        );

};

MainAssistant.prototype.callbackFunction = function(payload, item)
{
	//for (p in payload) alert(p + ': ' + payload[p]);
	
	if (!payload) 
	{
		this.alertMessage('SysHealth', $L("Cannot access the service. Have you installed it? If so, reboot your phone and try again."));
	}
	else if (payload.errorCode == -1) 
	{
		if (payload.errorText == "org.webosinternals.syshealth is not running.") 
		{
			this.alertMessage('SysHealth', $L("The service is not running. Have you installed it? If so, reboot your phone and try again."));
		}
		else 
		{
			this.alertMessage('SysHealth', payload.errorText);
		}
	}
	else if (payload.errorCode == "ErrorGenericUnknownMethod") 
	{
		this.alertMessage('SysHealth', $L("The service version is too old. First try rebooting your phone, or reinstall it and try again."));
	}
	else 
	{
		if (payload.apiVersion && payload.apiVersion < this.ipkgServiceVersion) 
		{
			this.alertMessage('SysHealth', $L("The service version is too old. First try rebooting your phone, or reinstall it and try again."));
		}
	}
	
	if (item) this.controller.get(item).mojo.deactivate();
}

MainAssistant.prototype.doAction = function(action)
{
	try
	{
		var r = new Mojo.Service.Request
		(
			'palm://org.webosinternals.syshealth',
			{
				method: action,
				onSuccess: this.callbackFunction.bindAsEventListener(this, action),
				onFailure: this.callbackFunction.bindAsEventListener(this, action)
			}
		);
	}
	catch (e)
	{
		Mojo.Log.logException(e, 'main#doAction');
		this.alertMessage('main#doAction Error', e);
	}
}

MainAssistant.prototype.alertMessage = function(title, message)
{
	this.controller.showAlertDialog(
	{
	    onChoose: function(value) {},
		allowHTMLMessage: true,
	    title: title,
	    message: message,
	    choices:[{label:$L('Ok'), value:""}]
    });
}

MainAssistant.prototype.activate = function(event)
{
	if (this.controller.stageController.setWindowOrientation)
	{
    	this.controller.stageController.setWindowOrientation("up");
	}
	
	if (this.firstActivate)
	{
		
	}
	else
	{
		
	}
	this.firstActivate = true;
};
MainAssistant.prototype.deactivate = function(event)
{
};
MainAssistant.prototype.visible = function(event)
{
	if (!this.isVisible)
	{
		this.isVisible = true;
	}
};
MainAssistant.prototype.invisible = function(event)
{
	this.isVisible = false;
};

MainAssistant.prototype.getRandomSubTitle = function()
{
	// loop to get total weight value
	var weight = 0;
	for (var r = 0; r < this.randomSub.length; r++)
	{
		weight += this.randomSub[r].weight;
	}
	
	// random weighted value
	var rand = Math.floor(Math.random() * weight);
	//alert('rand: ' + rand + ' of ' + weight);
	
	// loop through to find the random title
	for (var r = 0; r < this.randomSub.length; r++)
	{
		if (rand <= this.randomSub[r].weight)
		{
			return this.randomSub[r].text;
		}
		else
		{
			rand -= this.randomSub[r].weight;
		}
	}
	
	// if no random title was found (for whatever reason, wtf?) return first and best subtitle
	return this.randomSub[0].text;
};

MainAssistant.prototype.handleCommand = function(event)
{
	if (event.type == Mojo.Event.command)
	{
		switch (event.command)
		{
			case 'do-prefs':
				this.controller.stageController.pushScene('preferences');
				break;
				
			case 'do-help':
				this.controller.stageController.pushScene('help');
				break;
		}
	}
};

MainAssistant.prototype.cleanup = function(event)
{
	this.controller.stopListening(this.controller.stageController.document, Mojo.Event.stageActivate,   this.visible);
	this.controller.stopListening(this.controller.stageController.document, Mojo.Event.stageDeactivate, this.invisible);
};

// Local Variables:
// tab-width: 4
// End:
