webform_validate_progress = {};

Drupal.behaviors.mywebform_validate = function (context) {
	// Live fix values for custom and money fields
	$('input').live('change', function() {
		var fieldName = $(this).attr('field');
		
		if(fieldName) {
			switch(Drupal.settings.mywebform.fields[fieldName].type) {
				case 'custom':
					var val = $(this).val();
					if(val.length) {
						$(this).val(sprintf($(this).attr('format'), val));
					}
					break;
				case 'money':
					var val = $(this).val();
					if(val.length) {
						val = formatNumber(val, 2);
						$(this).val(val);
					}
					break;
                case 'float':
                  var val = $(this).val();
                  var decimal_length = $(this).attr('decimal_length');
                  if(parseInt(decimal_length) <= 0) {
                    decimal_length = 2;
                  }
                  if(val.length) {
                    val = formatNumber(val, decimal_length);
                    $(this).val(val);
                  }
                break;
			}
		}
	});
	
	// Validate webform on submit
	$('#mywebform-edit-form .form-submit').click(function(evt) {
		var period = -1;
		var period_elem = $('input.period');
		if(!period_elem.length) {
			period_elem = $('select.period');
		}
		if(period_elem.length) {
			period = period_elem.val();
		}
		
		var period_valid = false;
		if(period == -1) {
			period_valid = true;
		} else {
			var msg = Drupal.settings.mywebform.messages.confirm_period;
			period_valid = confirm(msg.replace('@period', period));
		}
		
		if(period_valid) {
			webform.errors = [];
			webform.warnings = [];
			webform_validate_progress = {};
			// Se all validators progress to 0. Their functions will change the corresponding progress value.
			for(x in webform.validators) {
				webform_validate_progress[x] = 0;
			}
		
			validateWebform();
		}
		return false;
	});
};

function validateWebform() {
	$('input, select').removeClass('error');
	
	for(x in webform.validators) {
		if(!webform_validate_progress[x]) {
			setTimeout('webform.validators.' + x + '();', 1);
			return;
		}
	}
	
	var has_errors = webform.errors.length > 0 ? true : false;
	show_error(has_errors);
	
	if(has_errors) {
		var warning = '';
		for(x in webform.warnings) {
			if(webform.warnings[x].msg != '') {
				var field = Drupal.settings.mywebform.fields[webform.warnings[x].fieldName];
				var msg = field.title != '' ? field.title : field.name;
				if(field.grid_name != '') {
					msg += '[' + (parseInt(webform.warnings[x].index) + 1) + ']';
				}
				msg += ' - ' + webform.warnings[x].msg;
				warning += msg + "\n";
			}
		}
	} else {
		$('#warnings').val(warning);
		$('tr.drow').remove();
		$('#js_ok').val('1');
		$('#mywebform-edit-form').submit();
	}
}

function show_error(has_errors) {
	var error_list = '';
	
	if(has_errors) {
		for(x in webform.errors) {
			var field = Drupal.settings.mywebform.fields[webform.errors[x].fieldName];
			
			// Add error to list
			if(webform.errors[x].msg != '') {
				if(field) {
					var msg = field.title != '' ? field.title : field.name;
					if(field.grid_name != '') {
						msg += '[' + (parseInt(webform.errors[x].index) + 1) + ']';
					}
					msg += ' - ' + webform.errors[x].msg;
				} else {
					msg = webform.errors[x].msg;
				}
				error_list += '<li>' + msg + "</li>\n";
			}
			
			// Mark failed field with error class
			if(field) {
				if(field.grid_name == '') {
					var elem = $('#' + webform.errors[x].fieldName);
				} else {
					var elem = $('#' + webform.errors[x].fieldName + '-' + (parseInt(webform.errors[x].index) + 1));
				}
				if(elem.length && !elem.hasClass('error')) elem.addClass('error');
			}
		}
		
		$('#mywebform-error').html('<ul>' + error_list + '</ul>');
		$('#mywebform-error').show();
		$('html, body').animate({scrollTop:0}, 'slow');
	} else {
		$('#mywebform-error').hide();
	}
}

function validate_fiscal_period() {
	var period = -1;
	var period_elem = $('input.period');
	if(!period_elem.length) {
		period_elem = $('select.period');
	}
	if(period_elem.length) {
		period = period_elem.val();
	}
	
	if(period == '') {
		var fieldName = period_elem.attr('field');
		webform.errors.push({'fieldName' : fieldName, 'index' : 0, 'msg' : Drupal.t('Fiscal period must not be empty', {})});
	} else if(period == -1) {
	} else {
		var month = 0;
		var year = 0;
		var period_start = new Date(Drupal.settings.mywebform.period.start.ts * 1000);
		var period_end = new Date(Drupal.settings.mywebform.period.end.ts * 1000);
		
		period = period.split('/');
		switch(period[0]) {
			case 'A':
				month = 0;
				year = parseInt(period[1]);
				break;
			case 'L':
				month = parseInt(period[1].replace(/^0+/, '')) - 1;
				year = parseInt(period[2]);
				break;
			case 'T':
				month = (parseInt(period[1]) - 1) * 3;
				year = parseInt(period[2]);
				break;
			case 'S':
				month = (parseInt(period[1]) - 1) * 6;
				year = parseInt(period[2]);
				break;
			case 'F':
				month = (parseInt(period[1]) - 1) * 4;
				year = parseInt(period[2]);
				break;
		}
		
		period = new Date(year, month, 1, 12, 0, 0, 0);
		if(period < period_start || period > period_end) {
			var fieldName = period_elem.attr('field');
			webform.errors.push({'fieldName' : fieldName, 'index' : 0, 'msg' : Drupal.t('Wrong fiscal period', {})});
		}
	}
}

webform.validators.validateForm = function () {
	// Check fiscal period
	validate_fiscal_period();
	
	// Get the count of valid rows in each grid. Ignore row-index fields if the
	// grid has a single row.
	var grid_values_count = {};
	for(gridName in Drupal.settings.mywebform.grids) {
		var cnt = 0;
		for(x in Drupal.settings.mywebform.grids[gridName].fields) {
			var fieldName = Drupal.settings.mywebform.grids[gridName].fields[x];
			var field = Drupal.settings.mywebform.fields[fieldName];
			var value = Drupal.settings.mywebform.values[fieldName];
			if(value.length > 1) {
				cnt = value.length;
			} else {
				if(field['class'].indexOf('row-index') < 0 && value[0] != '' && value[0] != null) {
					cnt = 1;
				}
			}
		}
		grid_values_count[gridName] = cnt;
	}
	
	// Check the values
	for(fieldName in Drupal.settings.mywebform.values) {
		var field = Drupal.settings.mywebform.fields[fieldName];
		var value = Drupal.settings.mywebform.values[fieldName];
		
		if(field.grid_name == '') {
			value = [value];
		} else {
			// Skip checking for grid rows if the grid its empty
			if(!grid_values_count[field.grid_name]) continue;
		}

		for(i in value) {
			if(value[i] == null) value[i] = '';

            if(field.required == true) {
              if(field.type == 'numeric') {
                if(value[i] === '') {
                  webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : Drupal.t('This field is required')});
                }
              }
              else {
                if(value[i] == '') {
                  webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : Drupal.t('This field is required')});
                }
              }
            }
            			
			if(field.minlength > 0 && value[i].length < field.minlength) {
				webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : Drupal.t('This field must have at least @length symbols', {'@length' : field.minlength})});
			}
		
			if(field.maxlength > 0 && value[i].length > field.maxlength) {
				webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : Drupal.t('This field must have not more than @length symbols', {'@length' : field.maxlength})});
			}
			
			if(value[i] != '' && field.type) {
				var pattern = '';
				var msg = '';
				
				switch(field.type) {
					case 'period':
						pattern = "^((a|A)\\/\\d{4}$)|((t|T)\\/[1234]\\/\\d{4}$)|((s|S)\\/[12]\\/\\d{4}$)|((l|L)\\/\\d{1,2}\\/\\d{4})|((f|F)\\/[123]\\/\\d{4}$)$";
						msg = Drupal.t('Wrong field format: period needed');
						break;
					case 'latintext':
						pattern = "^[A-Za-z0-9.,]*$";
						msg = Drupal.t('Wrong field format: latin text needed');
						break;	
					case 'numeric':
						pattern = "^([-][0-9]{1})?[0-9]*?$";
						msg = Drupal.t('Wrong field format: natural number needed');
						break;	
					case 'float':
						pattern = "^([-][0-9]{1})?[0-9]*([.,,][0-9]+)?$";
						msg = Drupal.t('Wrong field format: float number needed');
						break;	
				}
				
				if (pattern) {
					pattern = new RegExp(pattern);
					if(!pattern.test(value[i])) {
						webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : msg});
					}
				}
				
				if(field.type == 'date') {
					var val = value[i].replace('/', '.').split(".");
					var dayfield = val[0];
					var monthfield = val[1];
					var yearfield = val[2];
					if(!checkdate(monthfield, dayfield, yearfield)) {
						webform.errors.push({'fieldName' : fieldName, 'index' : i, 'msg' : Drupal.t('Wrong field format: date needed')});
					}
				}
			}
		}
	}
	
	webform_validate_progress['validateForm'] = 1;
	validateWebform();
}

function validationMarkFields() {
	for(x in webform.errors) {
		var field = Drupal.settings.mywebform.fields[webform.errors[x].fieldName];
		if(field.grid_name == '') {
			var elem = $('#' + webform.errors[x].fieldName);
		} else {
			var elem = $('#' + webform.errors[x].fieldName + '-' + (parseInt(webform.errors[x].index) + 1));
		}
		if(elem.length && !elem.hasClass('error')) elem.addClass('error');
	}
}

