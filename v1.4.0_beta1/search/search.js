var serverUrl="file://C:/SL/DOC TOOLS/SLP DOCOUT/searchengine.html";
var tagMap = {
};

function SearchBox(name, resultsPath, inFrame, label)
{
  this.searchLabel = label;
  this.DOMSearchField = function()
  {  return document.getElementById("MSearchField");  }
  this.DOMSearchBox = function()
  {  return document.getElementById("MSearchBox");  }
  this.OnSearchFieldFocus = function(isActive)
  {
    if (isActive)
    {
      this.DOMSearchBox().className = 'MSearchBoxActive';
      var searchField = this.DOMSearchField();
      if (searchField.value == this.searchLabel)
      {
        searchField.value = '';
      }
    }
    else
    {
      this.DOMSearchBox().className = 'MSearchBoxInactive';
      this.DOMSearchField().value   = this.searchLabel;
    }
  }
}

function trim(s) {
  return s?s.replace(/^\s\s*/, '').replace(/\s\s*$/, ''):'';
}

function getURLParameter(name) {
  return decodeURIComponent((new RegExp('[?|&]'+name+
         '='+'([^&;]+?)(&|#|;|$)').exec(location.search)
         ||[,""])[1].replace(/\+/g, '%20'))||null;
}

var entityMap = {
  "&": "&amp;",
  "<": "&lt;",
  ">": "&gt;",
  '"': '&quot;',
  "'": '&#39;',
  "/": '&#x2F;'
};

function escapeHtml(s) {
  return String(s).replace(/[&<>"'\/]/g, function (s) {
    return entityMap[s];
  });
}

function searchFor(query,page,count) {
    query = query.toLowerCase();
    if(trim(query).length<3)
    {
        var results = $('#searchresults');
        results.html('<p>Query too short (3 chars mininum).</p>');
        return;
    }

    var xmlData=document.getElementById('searchdata').innerHTML;
    if(xmlData.indexOf("<add>")<0)
    {
        alert("Append <script id=\"searchdata\" type=\"text/xmldata\"> to the content of search.html, then the content of searchdata.xml, and end by </script>.\n\nsearchdata.xml is generated when SEARCHENGINE, SERVER_BASED_SEARCH and EXTERNAL_SEARCH options are enabled.");
        return;
    }
    xmlData=xmlData.substring(xmlData.indexOf("<add>"),xmlData.indexOf("</add>")+6);

    var xmlParser=new DOMParser().parseFromString(xmlData,"text/xml");

    count=0;
    tableresult='<table>'; // Whole table content
    // Table rows per type
    var rows_per_type = {}

    var doc=xmlParser.getElementsByTagName("doc");
    for (i=0;i<doc.length;i++)
    {
        type="";
        name="";
        args="";
        url="";
        keywords="";
        text="";

        for (j=0; j<doc[i].children.length; j++)
        {
            child = doc[i].children[j];
            fieldname = child.getAttribute("name");
            switch (fieldname)
            {
                case "type":
                    type = child.innerHTML;
                    break;
                case "name":
                    name = child.innerHTML;
                    break;
                case "args":
                    args = child.innerHTML;
                    break;
                case "url":
                    url = child.innerHTML;
                    break;
                case "keywords":
                    keywords=child.innerHTML;
                    break;
                case "text":
                    text = child.innerHTML;
                    break;
                default:
                    break;
            }
        }
		
		// Special case, if type=='page' and 'text' is empty, change 'type' to
		// 'example'. Then, results will be handled differently
		if (type=="page" && (text.length == 0))
		{
			type="example";
		}

        // Search from name, args, keywords and text
        is_match = false;
        // Do not match the whole file in 'source' because it will flood...
        if (name.toLowerCase().includes(query) && (type != "source"))
        {
            is_match = true;
            match = name;
        }
        else if (keywords.toLowerCase().includes(query))
        {
            is_match = true;
            match = keywords;
        }
        else if (text.toLowerCase().includes(query))
        {
            is_match = true;
            match = text;
        }
        else if (args.toLowerCase().includes(query))
        {
            is_match = true;
            match = args;
        }

        if (is_match)
        {
            count++;
            output='<tr class="searchresult">';
            output+='<td>'+escapeHtml(type)+'&#160;';
            output+='<a href="'+escapeHtml(url)+'">';
            output+=escapeHtml(name);
            output+='</a>';
            output+='</td>';

            var start=match.toLowerCase().indexOf(query.toLowerCase());
            var fragmentcount=0;
            while(start>=0 && fragmentcount<3)
            {
                quotestart=Math.max(start-30,0);
                quoteend=Math.min(start+query.length+30,match.length);
                fragment='';
                if(quotestart>0)
                    fragment+='...';
                fragment+=escapeHtml(match.substring(quotestart,start));
                fragment+='<span class="hl">';
                fragment+=escapeHtml(match.substring(start,start+query.length));
                fragment+='</span>';
                fragment+=escapeHtml(match.substring(start+query.length,quoteend));
                if(quoteend<query.length);
                    fragment+='...';
                output+='<tr><td>'+fragment+'</td></tr>';

                start=match.toLowerCase().indexOf(query.toLowerCase(),start+1);
                fragmentcount++;
            }

            output+="</tr>";
            // Add type to tableresult if not present
            if (!(type in rows_per_type))
            {
                rows_per_type[type] = "";
            }
            rows_per_type[type]+=output;
        }
    }

    // Compose results, in correct order, most important ones firstChild
    if ("file" in rows_per_type)
    {
        tableresult+=rows_per_type["file"];
    }
    if ("page" in rows_per_type)
    {
        tableresult+=rows_per_type["page"];
    }
    if ("typedef" in rows_per_type)
    {
        tableresult+=rows_per_type["typedef"];
    }
    if ("function" in rows_per_type)
    {
        tableresult+=rows_per_type["function"];
    }
    if ("struct" in rows_per_type)
    {
        tableresult+=rows_per_type["struct"];
    }
    if ("union" in rows_per_type)
    {
        tableresult+=rows_per_type["union"];
    }
    if ("enum" in rows_per_type)
    {
        tableresult+=rows_per_type["enum"];
    }
    if ("enumvalue" in rows_per_type)
    {
        tableresult+=rows_per_type["enumvalue"];
    }
    if ("define" in rows_per_type)
    {
        tableresult+=rows_per_type["define"];
    }
	if ("example" in rows_per_type)
	{
		tableresult+=rows_per_type["example"];
	}
    if ("source" in rows_per_type)
    {
        tableresult+=rows_per_type["source"];
    }

    tableresult+="</table>";
    var results = $('#searchresults');
    if (count==0) {
        searchres = '<p>Sorry, no matches for query ';
    } else if (count==1) {
		searchres = '<p>Found <b>1</b> match for query ';
    } else {
        searchres = '<p>Found <b>' + count + '</b> matches for query ';
    }
	searchres += '<i>' + query + '</i>:</p>';
	results.html(searchres);
    results.append(tableresult);
	results.append(`<p><b>Note</b>: Due to the limitations of Doxygen search indexing, results to documentation pages 
	cannot link directly to actual match.<br>Instead, the linking can only be  done to the beginning of the documentation page.
	<br>Then, you must use browser's <I>find within page</I> to find the actual match.</p>`);
	
}

$(document).ready(function() {
  var query = trim(getURLParameter('query'));
  if (query) {
    searchFor(query,0,20);
  } else {
    var results = $('#results');
    results.html('<p>Sorry, no documents matching your query.</p>');
  }
});
