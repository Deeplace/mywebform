  var jscal;
  var webformValidator = {'errors' : {}, 'validators' : {}};
  var webform = {'afterLoad' : {}};

Drupal.behaviors.mywebform = {
  attach: function (context) {
  // Show print dialog
    if(Drupal.settings.mywebform.instantprint) {
      //window.print();
    }

    if(!Drupal.settings.mywebform.preview) {
      create_multistep();
    }
    // Print button
    jQuery('.forprint a').click(function(evt) {
      evt.preventDefault();
      var href = jQuery(this).attr('href');
      window.open(href, 'new', 'width=800,height=700,toolbar=1,scrollbars=1,resizable=1');
      return false;
    });

    // Add jscal to date inputs
    if(!Drupal.settings.mywebform.preview) {
      jscal = Calendar.setup({onSelect: function(jscal) {jscal.hide()}, dateFormat : "%d.%m.%Y"});
      jQuery('input.date').each(function () {
        var id = jQuery(this).attr('id');
        if(id) {
          jscal.manageFields(id, id, "%d.%m.%Y");
        }
      });
    }

    jQuery('a.goback').click( function () {
      history.go(-1);
      return false;
    });

    // Disable form submit on enter
    var textboxes = jQuery("input:text");
    if(jQuery.browser.mozilla) {
      jQuery(textboxes).keypress(checkForEnter);
    } else {
      jQuery(textboxes).keydown(checkForEnter);
    }

    function checkForEnter(evt) {
      if (evt.keyCode == 13) {
        var currentTextboxNumber = textboxes.index(this);
        if (textboxes[currentTextboxNumber + 1] != null) {
          nextTextbox = textboxes[currentTextboxNumber + 1];
          nextTextbox.select();
        }
        evt.preventDefault();
        return false;
      }
    }

    // Fill select boxes with values if they have appended containers
    jQuery('select').live('focus', function () {
      var obj = jQuery(this);
      if(obj.hasClass('filled')) return;
      if(typeof obj.attr('container') == 'undefined') return;
      if(obj.attr('container') == '') return;

      //var ta = new Date().getTime();
      obj.addClass('filled');

      var elem = document.getElementById(obj.attr('id'));
      var selectedValue = elem.options[elem.selectedIndex].value;
      var i = 0;

      obj.empty();
      try{
        jQuery.each(window[obj.attr('container')], function (key, item) {
          elem.add(new Option(item.value, item.key), null);
          if(item.key == selectedValue) elem.selectedIndex = i;
          i++;
        });
      } catch(e) {
        jQuery.each(window[obj.attr('container')], function (key, item) {
          elem.add(new Option(item.value, item.key));
          if(item.key == selectedValue) elem.selectedIndex = i;
          i++;
        });
      }
      //var tb = new Date().getTime();
      //console.log((tb - ta) + 'ms');
    });

    jQuery('input[type=checkbox]').live('click', function() {
      if(jQuery(this).attr('radio')) {
        radio_checkbox(jQuery(this));
      }
    });

    if(!Drupal.settings.mywebform.edit) {
      fix_radio_group();
    }

    jQuery('#mywebform-content').bind('document_load', transform_documents_attach);

    fix_documents_for_print();

    jQuery('input[help_url], textarea[help_url]').each(function(){
      jQuery(this).after('<span class="help-link" help_id="' + jQuery(this).attr('id') + '"></span>');
    });

    jQuery('input[help_url], textarea[help_url]').each(function(){
      jQuery('#mywebform-edit-form').append('<div style="display:none"><div class="help-dialog" dial_id="' + jQuery(this).attr('id') + '">' + jQuery(this).attr('help_url') +'</div></div>');
    });

    if(jQuery('.help-dialog').length > 0) {
      jQuery('.help-dialog').dialog({
        autoOpen: false,
        show: 'blind',
        hide: 'explode',
        minWidth: 300
      });
    }
    jQuery('.help-link').live('click', function(){
      jQuery('.help-dialog[dial_id="' + jQuery(this).attr('help_id') + '"]').dialog('open');
      return false;
    });

    if(jQuery('#mywebform-multistep ul li').length > 0) {
      var mul_max_height = 0;
      jQuery('#mywebform-multistep ul li', context).each(function(){
        if(jQuery(this).height() > mul_max_height) {
          mul_max_height = jQuery(this).height();
        }
      });
      jQuery('#mywebform-multistep ul li', context).css('height', mul_max_height);
    }
  }
};

function add_required_field_image(elem) {
  if(elem.next().is('span.form-required')) {
    return;
  }
  elem.after('<span class="form-required">*</span>');
}

function remove_required_field_image(elem) {
  if(elem.next().is('span.form-required')) {
    elem.next().remove();
  }
}

// Special functions to user in autofields
function sum(field) {
  var sum_value = 0.0;
  var autofield_func = field.attr('autofield_func');
  jQuery('input[type=text]').each(function() {
    if(jQuery(this).attr('autofield_func') == autofield_func) {
      sum_value += toFloat(jQuery(this));
    }
  });

  return sum_value;
}

function min(value1, value2) {
  var v1 = parseFloat(value1);
  var v2 = parseFloat(value2);
  var result = (v1 <= v2) ? v1 : v2;
  return isNaN(result) ? 0 : result;
}

function positive_only(value) {
  if(isNaN(value)) return 0;
  return value >0 ? value : 0;
}

function formatNumber(num, numAfterPoint) {
  var str = num.toString();
  str = str.replace(',', '.');
  if(isNaN(str)) str = new Number(0);
  var formatNum = new Number(str);
  return formatNum.toFixed(numAfterPoint);
}

function toFloat(obj) {
  var val = parseFloat(obj.val().replace(',', '.'));
  if(isNaN(val)) val = 0.0;
  return val;
}

// Converts a group of checkboxes into a radio set
function radio_checkbox(obj) {
  obj.siblings('input:checkbox').each(function() {
    if(jQuery(this).attr('rel') == obj.attr('rel')) {
      jQuery(this).attr('checked', false);
    }
  });
  obj.attr('checked', true);
}

function fix_radio_group() {
  jQuery('input:checkbox').each(function(){
    if(jQuery(this).attr('radio') == 'on') {
      if(jQuery(this).siblings('input[rel="' + jQuery(this).attr('rel') +'"]:checked').length == 0) {
        jQuery(this).trigger('click').attr('checked', 'checked');
      }
    }
  });
}
// call sprintf("Hello {1} {2}", "Pavlic", "Morozov");
function ms_sprintf() {
  var num = arguments.length;
  var oStr = arguments[0];
  for (var i = 1; i < num; i++) {
    var pattern = "\\{" + i + "\\}";
    var re = new RegExp(pattern, "g");
    oStr = oStr.replace(re, arguments[i]);
  }
  return oStr;
}

// php sprintf emulation from phpjs.org
function sprintf () {
    // http://kevin.vanzonneveld.net
    // +   original by: Ash Searle (http://hexmen.com/blog/)
    // + namespaced by: Michael White (http://getsprink.com)
    // +    tweaked by: Jack
    // +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +      input by: Paulo Freitas
    // +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +      input by: Brett Zamir (http://brett-zamir.me)
    // +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // *     example 1: sprintf("%01.2f", 123.1);
    // *     returns 1: 123.10
    // *     example 2: sprintf("[%10s]", 'monkey');
    // *     returns 2: '[    monkey]'
    // *     example 3: sprintf("[%'#10s]", 'monkey');
    // *     returns 3: '[####monkey]'
    var regex = /%%|%(\d+\$)?([-+\'#0 ]*)(\*\d+\$|\*|\d+)?(\.(\*\d+\$|\*|\d+))?([scboxXuidfegEG])/g;
    var a = arguments,
        i = 0,
        format = a[i++];

    // pad()
    var pad = function (str, len, chr, leftJustify) {
        if (!chr) {
            chr = ' ';
        }
        var padding = (str.length >= len) ? '' : Array(1 + len - str.length >>> 0).join(chr);
        return leftJustify ? str + padding : padding + str;
    };

    // justify()
    var justify = function (value, prefix, leftJustify, minWidth, zeroPad, customPadChar) {
        var diff = minWidth - value.length;
        if (diff > 0) {
            if (leftJustify || !zeroPad) {
                value = pad(value, minWidth, customPadChar, leftJustify);
            } else {
                value = value.slice(0, prefix.length) + pad('', diff, '0', true) + value.slice(prefix.length);
            }
        }
        return value;
    };

    // formatBaseX()
    var formatBaseX = function (value, base, prefix, leftJustify, minWidth, precision, zeroPad) {
        // Note: casts negative numbers to positive ones
        var number = value >>> 0;
        prefix = prefix && number && {
            '2': '0b',
            '8': '0',
            '16': '0x'
        }[base] || '';
        value = prefix + pad(number.toString(base), precision || 0, '0', false);
        return justify(value, prefix, leftJustify, minWidth, zeroPad);
    };

    // formatString()
    var formatString = function (value, leftJustify, minWidth, precision, zeroPad, customPadChar) {
        if (precision != null) {
            value = value.slice(0, precision);
        }
        return justify(value, '', leftJustify, minWidth, zeroPad, customPadChar);
    };

    // doFormat()
    var doFormat = function (substring, valueIndex, flags, minWidth, _, precision, type) {
        var number;
        var prefix;
        var method;
        var textTransform;
        var value;

        if (substring == '%%') {
            return '%';
        }

        // parse flags
        var leftJustify = false,
            positivePrefix = '',
            zeroPad = false,
            prefixBaseX = false,
            customPadChar = ' ';
        var flagsl = flags.length;
        for (var j = 0; flags && j < flagsl; j++) {
            switch (flags.charAt(j)) {
            case ' ':
                positivePrefix = ' ';
                break;
            case '+':
                positivePrefix = '+';
                break;
            case '-':
                leftJustify = true;
                break;
            case "'":
                customPadChar = flags.charAt(j + 1);
                break;
            case '0':
                zeroPad = true;
                break;
            case '#':
                prefixBaseX = true;
                break;
            }
        }

        // parameters may be null, undefined, empty-string or real valued
        // we want to ignore null, undefined and empty-string values
        if (!minWidth) {
            minWidth = 0;
        } else if (minWidth == '*') {
            minWidth = +a[i++];
        } else if (minWidth.charAt(0) == '*') {
            minWidth = +a[minWidth.slice(1, -1)];
        } else {
            minWidth = +minWidth;
        }

        // Note: undocumented perl feature:
        if (minWidth < 0) {
            minWidth = -minWidth;
            leftJustify = true;
        }

        if (!isFinite(minWidth)) {
            throw new Error('sprintf: (minimum-)width must be finite');
        }

        if (!precision) {
            precision = 'fFeE'.indexOf(type) > -1 ? 6 : (type == 'd') ? 0 : undefined;
        } else if (precision == '*') {
            precision = +a[i++];
        } else if (precision.charAt(0) == '*') {
            precision = +a[precision.slice(1, -1)];
        } else {
            precision = +precision;
        }

        // grab value using valueIndex if required?
        value = valueIndex ? a[valueIndex.slice(0, -1)] : a[i++];

        switch (type) {
        case 's':
            return formatString(String(value), leftJustify, minWidth, precision, zeroPad, customPadChar);
        case 'c':
            return formatString(String.fromCharCode(+value), leftJustify, minWidth, precision, zeroPad);
        case 'b':
            return formatBaseX(value, 2, prefixBaseX, leftJustify, minWidth, precision, zeroPad);
        case 'o':
            return formatBaseX(value, 8, prefixBaseX, leftJustify, minWidth, precision, zeroPad);
        case 'x':
            return formatBaseX(value, 16, prefixBaseX, leftJustify, minWidth, precision, zeroPad);
        case 'X':
            return formatBaseX(value, 16, prefixBaseX, leftJustify, minWidth, precision, zeroPad).toUpperCase();
        case 'u':
            return formatBaseX(value, 10, prefixBaseX, leftJustify, minWidth, precision, zeroPad);
        case 'i':
        case 'd':
            number = (+value) | 0;
            prefix = number < 0 ? '-' : positivePrefix;
            value = prefix + pad(String(Math.abs(number)), precision, '0', false);
            return justify(value, prefix, leftJustify, minWidth, zeroPad);
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            number = +value;
            prefix = number < 0 ? '-' : positivePrefix;
            method = ['toExponential', 'toFixed', 'toPrecision']['efg'.indexOf(type.toLowerCase())];
            textTransform = ['toString', 'toUpperCase']['eEfFgG'.indexOf(type) % 2];
            value = prefix + Math.abs(number)[method](precision);
            return justify(value, prefix, leftJustify, minWidth, zeroPad)[textTransform]();
        default:
            return substring;
        }
    };

    return format.replace(regex, doFormat);
}

function in_array (needle, haystack, argStrict) {
    // Checks if the given value exists in the array
    //
    // version: 1103.1210
    // discuss at: http://phpjs.org/functions/in_array    // +   original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +   improved by: vlado houba
    // +   input by: Billy
    // +   bugfixed by: Brett Zamir (http://brett-zamir.me)
    // *     example 1: in_array('van', ['Kevin', 'van', 'Zonneveld']);    // *     returns 1: true
    // *     example 2: in_array('vlado', {0: 'Kevin', vlado: 'van', 1: 'Zonneveld'});
    // *     returns 2: false
    // *     example 3: in_array(1, ['1', '2', '3']);
    // *     returns 3: true    // *     example 3: in_array(1, ['1', '2', '3'], false);
    // *     returns 3: true
    // *     example 4: in_array(1, ['1', '2', '3'], true);
    // *     returns 4: false
    var key = '',        strict = !! argStrict;

    if (strict) {
        for (key in haystack) {
            if (haystack[key] === needle) {                return true;
            }
        }
    } else {
        for (key in haystack) {            if (haystack[key] == needle) {
                return true;
            }
        }
    }
    return false;
}

function create_multistep() {
  var index = 0;
  jQuery('#mywebform-edit-form').prepend('<div id="mywebform-multistep"><ul></ul></div><div class="clear-both"></div>');
  jQuery('#mywebform-edit-form fieldset').each(function(){
    var content;
    if(jQuery(this).parent('fieldset').length == 0) {
      index++;
      jQuery('#mywebform-multistep ul').append('<li><a menuindex = "step-' + index + '" class = "step-' + index + '" href="#step-' + index + '">' + jQuery(this).children('legend').text() + '</a></li>')
      jQuery(this).children('legend').remove();
      content = jQuery(this).html();
      jQuery(this).after('<div style="display:none" class="multistep-container" id="step-' + index + '">' + content + '</div>');
      jQuery(this).remove();
    }
  });

  if(jQuery('#documents-user-type-form').length > 0) {
    jQuery('.representant-type input[type!="checkbox"]').attr('value', '');
    jQuery('.solicitant-type input[type!="checkbox"]').attr('value', '');
    jQuery('.representant-type select :first').attr('selected', 'selected');
    jQuery('.solicitant-type select :first').attr('selected', 'selected');

    if(typeof jQuery.uniform.update == 'function') {
      jQuery.uniform.update('.representant-type select');
      jQuery.uniform.update('.solicitant-type select');
    }
  }

  jQuery('#mywebform-edit-form #step-1').show();
  jQuery('#mywebform-multistep li a.step-1,#mywebform-multistep li:has(a.step-1)').addClass('active');

  if(index != 0) {
    jQuery('#step-' + index).after('<div id="multistep-navigation-inline-wrapper"><div id="multistep-navigation-inline"><ul><li><a class="button next-button" id="next-form" menuindex = "step-2" href="#step-2" current_step="#step-1"><span><span>'+ Drupal.t('Next') +'</span></span></a></li><li><span class="next-step">' + jQuery('#mywebform-multistep a[menuindex="step-2"]').html() + '</span></li></ul></div></div><div class="clear-both"></div><div class="multistep-separate"></div>');
  }

  jQuery('#multistep-navigation-inline a').live('click', function(){
    var ret = validateWebform(true, jQuery(jQuery(this).attr('current_step')));
    if(ret) {
      jQuery('#mywebform-error').empty().hide();
      jQuery('#mywebform-multistep li .' + jQuery(this).attr('menuindex')).trigger('click');
    }
    return false;
  });

  jQuery('#mywebform-multistep li a').live('click', function(){
    var cur_index = 0;
    var cur_menu_index = jQuery(this).attr('menuindex');
    var prev_item = 0;
    var next_item = 0;
    jQuery('.multistep-container').hide();
    jQuery('#mywebform-multistep li a, #mywebform-multistep li').removeClass('active');
    jQuery('#' + cur_menu_index).show();
    jQuery(this).addClass('active');
    jQuery(this).parents('li').addClass('active');
    jQuery('#multistep-navigation-inline').empty();
    jQuery('#multistep-navigation-inline').append('<ul></ul>');

    cur_index = parseInt(cur_menu_index.substr(5));
    if(cur_index && cur_index != 1) {
      prev_item = cur_index - 1;
      jQuery('#multistep-navigation-inline ul').append('<li><span class="previous-step">' + jQuery('#mywebform-multistep a[menuindex="step-'+ prev_item +'"]').html() + '</span></li>');
      jQuery('#multistep-navigation-inline ul').append('<li><a class="button previous-button" id="back-form" menuindex = "step-'+ prev_item +'" href="#step-' + prev_item +'" current_step="#step-'+ cur_index +'"><span><span>' + Drupal.t('Previos') + '</span></span></a></li>');
    }
    if((cur_index) && (cur_index < index)) {
      next_item = cur_index + 1;
      jQuery('#multistep-navigation-inline ul').append('<li><a class="button next-button" id="next-form" menuindex = "step-'+ next_item +'" href="#step-' + next_item +'" current_step="#step-'+ cur_index +'"><span><span>' + Drupal.t('Next') + '</span></span></a></li>');
      jQuery('#multistep-navigation-inline ul').append('<li><span class="next-step">' + jQuery('#mywebform-multistep a[menuindex="step-'+ next_item +'"]').html() + '</span></li>');
    }
    fix_navigation_inline();
    return false;
  });
  fix_navigation_inline();
}

function fix_navigation_inline() {
  var width = jQuery('#multistep-navigation-inline').outerWidth(true);
  var center = (jQuery('#mywebform-content').width() / 2) - (width / 2);
  jQuery('#multistep-navigation-inline-wrapper').css('margin-left', center);
}

function transform_documents_attach(preview) {
  jQuery('#DinamicDocuments .grid tr').hide();
  jQuery('#DinamicDocuments td.delrow').hide();
  jQuery('#DinamicDocuments a.grid-addrow').hide();
  var output = jQuery('#DinamicDocuments').after('<div id="DinamicDocumentsReplaced"><table><tbody><tr><td class="left"></td><td class="right"></td></tr></tbody></table></div>');
  var doc_exist = 0;
  var select_count = jQuery('#DinamicDocuments .grid tr:first select option').length;
  var inserted_count = 0;
  var boundary = parseInt(select_count/2);

  jQuery('#DinamicDocuments .grid tr:first select option').each(function(){
    if((jQuery(this).parents('optgroup').length > 0) || (jQuery(this).val().length <= 0) || (jQuery(this).val() == '1')) {
      return;
    }
    if(inserted_count <= boundary) {
      var pos = 'left';
    }
    else {
      var pos = 'right';
    }
    jQuery('#DinamicDocumentsReplaced tr td.' + pos).append('<div class="replaced-group"><input type="checkbox" value="' + jQuery(this).attr('value') + '"/><span>' + jQuery(this).text() + '</span></div>');
    inserted_count++;
  });

  jQuery('#DinamicDocuments .grid tr:first select optgroup').each(function(){
    if(inserted_count <= boundary) {
      var pos = 'left';
    }
    else {
      var pos = 'right';
    }
    jQuery('#DinamicDocumentsReplaced tr td.' + pos).append('<div class="optiongroup">' + jQuery(this).attr('label') + '</div>');
    jQuery(this).find('option').each(function(){
      if(jQuery(this).val() == '1') {
        return;
      }
      jQuery('#DinamicDocumentsReplaced tr td.' + pos).append('<div class="replaced-group"><input type="checkbox" value="' + jQuery(this).attr('value') + '"/><span>' + jQuery(this).text() + '</span></div>');
      inserted_count++;
    });
  });

  jQuery('#DinamicDocuments .grid tr:not(.drow) select option:selected').each(function(){
    var index = jQuery(this).parents('select').attr('row-index');
    var value = jQuery(this).val();
    if(inserted_count <= boundary) {
      var pos = 'left';
    }
    else {
      var pos = 'right';
    }
    if(jQuery(this).val() == '1') {
      if(Drupal.settings.mywebform.preview) {
        jQuery('#DinamicDocumentsReplaced tr td.' + pos).append('<div class="replaced-group"><input type="checkbox" row-index="' + index +'" value="' + jQuery(this).attr('value') + '"/><span>' + jQuery('#doc_dinamic_doc_another_type-' + index).text() + '</span></div>');
      }
      else {
        jQuery('#DinamicDocumentsReplaced tr td.' + pos).append('<div class="replaced-group"><input type="checkbox" row-index="' + index +'" value="' + jQuery(this).attr('value') + '"/><span>' + jQuery('#doc_dinamic_doc_another_type-' + index).val() + '</span></div>');
      }
      jQuery('#DinamicDocumentsReplaced .replaced-group input[row-index="' + index + '"]').attr('checked', 'checked').attr('row-index', index);
      if(Drupal.settings.mywebform.preview) {
        jQuery('#DinamicDocumentsReplaced .replaced-group input[row-index="' + index + '"]').next().after('<span class="file-link">' + jQuery('#DinamicDocuments #doc_dinamic_doc_document-' + index).html() + '</span>');
        jQuery('#DinamicDocumentsReplaced .replaced-group input[row-index="' + index + '"]').next().after('<span class="pages" row-index="' + index + '">' + jQuery('#doc_dinamic_doc_pages-' + index).text() + '</span>');
      }
      else {
        jQuery('#DinamicDocumentsReplaced .replaced-group input[row-index="' + index + '"]').next().after('<span class="file-link">' + jQuery('#DinamicDocuments tr.row-' + index + ' .file-info .filename').html() + '</span>');
        jQuery('#DinamicDocumentsReplaced .replaced-group input[row-index="' + index + '"]').next().after('<input class="pages" type="text" row-index="' + index + '" value="' + jQuery('#doc_dinamic_doc_pages-' + index).val() + '"/>');
      }
      inserted_count++;
    }
    else {
      jQuery('#DinamicDocumentsReplaced .replaced-group input[value="' + value + '"]').attr('checked', 'checked').attr('row-index', index);
      if(Drupal.settings.mywebform.preview) {
        jQuery('#DinamicDocumentsReplaced .replaced-group input[value="' + value + '"]').next().after('<span class="file-link">' + jQuery('#DinamicDocuments #doc_dinamic_doc_document-' + index).html() + '</span>');
        jQuery('#DinamicDocumentsReplaced .replaced-group input[value="' + value + '"]').next().after('<span class="pages" row-index="' + index + '">' + jQuery('#doc_dinamic_doc_pages-' + index).text() + '</span>');
      }
      else {
        jQuery('#DinamicDocumentsReplaced .replaced-group input[value="' + value + '"]').next().after('<span class="file-link">' + jQuery('#DinamicDocuments tr.row-' + index + ' .file-info .filename').html() + '</span>');
        jQuery('#DinamicDocumentsReplaced .replaced-group input[value="' + value + '"]').next().after('<input class="pages" type="text" row-index="' + index + '" value="' + jQuery('#doc_dinamic_doc_pages-' + index).val() + '"/>');
      }
    }
  });

  jQuery('#DinamicDocuments select option:selected').each(function(){
    if((jQuery(this).val() != '1') && (jQuery(this).val() != '')) {
      jQuery('#DinamicDocuments tr.drow select option[value="' + jQuery(this).val() + '"]').remove();
    }
  });

  jQuery('#DinamicDocumentsReplaced input[type="checkbox"]').bind('click', function(){
    if(jQuery(this).attr('row-index')) {
      return true;
    }
    if(jQuery(this).attr('checked') == true) {
      jQuery('#DinamicDocuments a.grid-addrow').trigger('click');
      var row_id = jQuery('#DinamicDocuments tr').last().prev().attr('class');
      jQuery(this).attr('row_id', row_id);
      jQuery('#DinamicDocuments tr.' + row_id + ' select option[value="' + jQuery(this).val() + '"]').attr('selected','selected');

      if(typeof jQuery.uniform.update == 'function') {
        jQuery.uniform.update(jQuery('#DinamicDocuments tr.' + row_id + ' select'));
      }


      jQuery('#DinamicDocuments tr.' + row_id + ' select').attr('disabled', 'disabled');
      jQuery('#DinamicDocuments tr.' + row_id + ' input[type!="hidden"]').each(function(){
        if(jQuery(this).parent().hasClass('document_another_type') && jQuery(this).parent().css('display') == 'none') {
          return;
        }
        jQuery(this).addClass('required');
        add_required_field_image(jQuery(this));
      });
    }
    else {
      if(jQuery(this).attr('row_id')) {
        jQuery('#DinamicDocuments tr.' + jQuery(this).attr('row_id')).remove();
      }
    }
  });

  if(!Drupal.settings.mywebform.preview) {
  jQuery('#DinamicDocumentsReplaced td.right').append('<div class="other-type"><a id="add-other-type" href="#">' + Drupal.t('Add other type') + '</a></div>');
    jQuery('#DinamicDocumentsReplaced #add-other-type').bind('click', function(){
      jQuery('#DinamicDocuments a.grid-addrow').trigger('click');
      var row_id = jQuery('#DinamicDocuments tr').last().prev().attr('class');
      jQuery('#DinamicDocuments tr.' + row_id + ' select option[value="1"]').attr('selected','selected');

      jQuery('#DinamicDocuments tr.' + row_id + ' select').trigger('change');
      jQuery('#DinamicDocuments tr.' + row_id + ' select').attr('disabled', 'disabled');
      jQuery('#DinamicDocuments tr.' + row_id + ' td.delrow').show();
      jQuery('#DinamicDocuments tr.' + row_id + ' input[type!="hidden"]').each(function(){
         if(jQuery(this).parent().hasClass('document_another_type') && jQuery(this).parent().css('display') == 'none') {
           return;
         }
         jQuery(this).addClass('required');
          add_required_field_image(jQuery(this));
        });
      return false;
    });
  }

  if(Drupal.settings.mywebform.preview) {
    jQuery('#DinamicDocumentsReplaced input[type="checkbox"]').attr('disabled', 'disabled');
  }
  jQuery('#DinamicDocumentsReplaced .replaced-group span').each(function(){
    if(jQuery(this).text() == 'prezentată pe suport hârtie' || jQuery(this).text() == 'prezentată pe suport electronic') {
      jQuery(this).parent().hide();
    }
  });
}

function submit_documents() {
  jQuery('#DinamicDocumentsReplaced input[type="text"]').each(function(){
    var index = parseInt(jQuery(this).attr('row-index'));
    jQuery('#DinamicDocuments #doc_dinamic_doc_pages-' + index).val(jQuery(this).val());
  });

  jQuery('#DinamicDocumentsReplaced input[type="checkbox"]').each(function(){
    var index = parseInt(jQuery(this).attr('row-index'));
    if((jQuery(this).attr('checked') != true) && (index > 0)) {
      jQuery('#DinamicDocuments tr.row-' + index).remove();
    }
  });

  jQuery('#DinamicDocuments select').removeAttr('disabled');
}

function fix_documents_for_print() {
  if(jQuery('.attached-documents-inline-left').length > 0) {
    var left_height = jQuery('.attached-documents-inline-left').height();
    var right_height = jQuery('.attached-documents-inline-right').height();
    if(left_height > right_height) {
      jQuery('.attached-documents-inline-right').css('height', left_height);
    }
    else {
      left_height = jQuery('.attached-documents-inline-left').css('height', right_height);
    }
  }
}