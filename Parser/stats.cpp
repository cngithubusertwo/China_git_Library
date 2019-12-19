#include <stdio.h>
#include <algorithm>
#include "stats.h"

// Increment stats summary item
void StatsSummaryItem::Inc(int conv_status) 
{
	occurrences++;

	switch(conv_status) {
		case STATS_CONV_UNDEFINED:
			conv_undef++;
			break;
		case STATS_CONV_NO_NEED:
			conv_no_need++;
			break;
		case STATS_CONV_OK:
			conv_ok++;
			break;
		case STATS_CONV_WARN:
			conv_warn++;
			break;
		case STATS_CONV_ERROR:
			conv_error++;
			break;
	}
}

void StatsSummaryItem::Inc(StatsSummaryItem *item)
{
    if(item == nullptr)
		return;

	occurrences += item->occurrences;

	conv_undef += item->conv_undef;
	conv_no_need += item->conv_no_need;
	conv_ok += item->conv_ok;
	conv_warn += item->conv_warn;
	conv_error += item->conv_error;

	complexity_very_low += item->complexity_very_low;
	complexity_low += item->complexity_low;
	complexity_medium += item->complexity_medium;
	complexity_high += item->complexity_high;
	complexity_very_high += item->complexity_very_high;

	if(!item->snippets.empty())
		snippets.push_back(item->snippets.front());
}

// Update summary status for single occurrence from detail
void StatsSummaryItem::UpdateSingleOccurrenceStatus(StatsDetailItem *sid)
{
    if(sid == nullptr)
		return;

	// Error status is most critical, it overrides all other statuses
	if(sid->conv_status == STATS_CONV_ERROR)
	{
		conv_undef = 0;
		conv_no_need = 0;
		conv_ok = 0;
		conv_warn = 0;
		conv_error = 1;
	}
	else
	// Conversion is successful (overwrites only no_need)
	if (sid->conv_status == STATS_CONV_OK && conv_undef == 0 && conv_warn == 0 && conv_error == 0)
	{
		conv_no_need = 0;
		conv_ok = 1;
	}
	else
	// No conversion is required
	if (sid->conv_status == STATS_CONV_NO_NEED && conv_undef == 0 && conv_ok == 0 && conv_warn == 0 && conv_error == 0)
		conv_no_need = 1;

	occurrences = 1;
}

// Update summary complexity for single occurrence from detail
void StatsSummaryItem::UpdateSingleOccurrenceComplexity(StatsDetailItem *sid)
{
    if(sid == nullptr)
		return;

	// Very high overrides other levels
	if(sid->complexity == STATS_CONV_VERY_HIGH)
	{
		complexity_very_low = 0;
	    complexity_low = 0;
	    complexity_medium = 0;
	    complexity_high = 0;
	    complexity_very_high = 1;
	}
	else if(sid->complexity == STATS_CONV_HIGH && complexity_very_high == 0)
	{
		complexity_very_low = 0;
	    complexity_low = 0;
	    complexity_medium = 0;
	    complexity_high = 1;
	}
	else if(sid->complexity == STATS_CONV_MEDIUM && complexity_very_high == 0 && complexity_high == 0)
	{
		complexity_very_low = 0;
	    complexity_low = 0;
	    complexity_medium = 1;
	}
	else if(sid->complexity == STATS_CONV_LOW && complexity_very_high == 0 && complexity_high == 0 &&
			complexity_medium == 0)
	{
		complexity_very_low = 0;
	    complexity_low = 1;
	}
	else if(sid->complexity == STATS_CONV_VERY_LOW && complexity_very_high == 0 && complexity_high == 0 &&
			complexity_medium == 0 && complexity_low == 0)
		complexity_very_low = 1;
}

// Get most critical status
int StatsSummaryItem::GetMostCritical()
{
	if(conv_error > 0)
		return STATS_CONV_ERROR;
	else
	if(conv_undef > 0)
		return STATS_CONV_UNDEFINED;
	else
	if(conv_warn > 0)
		return STATS_CONV_WARN;
	else
	if(conv_ok > 0)
		return STATS_CONV_OK;
	else
	if(conv_no_need > 0)
		return STATS_CONV_NO_NEED;

	return -1;
}

// Constructor/destructor
Stats::Stats() {}

Stats::~Stats() {}

// Log function call with all nested expressions
void Stats::LogFuncCall(Token *name, Token *end, std::string &cur_file)
{
    if(name == nullptr || end == nullptr)
		return;

	static bool first_call = true;

	const char *openMode = first_call ? "w" : "a";

	FILE *file = fopen(SQLEXEC_STAT_FILE, openMode);

    if(file != nullptr)
	{
		// Line and source name
		fprintf(file, "%s,%d,%.*s,", cur_file.c_str(), name->line, (int)name->len, name->str);

		std::string src, src_meta;
		std::string tgt, tgt_meta;

		Token *cur = name;

		// Build source and target strings
		while(true)
		{
			const char *meta = GetMetaIdent(cur);

			// Source expression
			if(!(cur->flags & TOKEN_INSERTED))
			{
                if(cur->str != nullptr)
				{
					src.append(cur->str, cur->len);

                    if(meta != nullptr)
						src_meta.append(meta);
					else
						src_meta.append(cur->str, cur->len);
				}
				else
				if(cur->chr == '\r' || cur->chr == '\n' || cur->chr == '\t')
				{
					src += ' ';
					src_meta += ' ';
				}
				else
				{
					src += cur->chr;
					src_meta += cur->chr;
				}
			}

			// Target expression
			if(!(cur->flags & TOKEN_REMOVED))
			{
                if(cur->t_str != nullptr)
				{
					tgt.append(cur->t_str, cur->t_len);

                    if(meta != nullptr)
						tgt_meta.append(meta);
					else
						tgt_meta.append(cur->t_str, cur->t_len);
				}
				else
                if(cur->str != nullptr)
				{
					tgt.append(cur->str, cur->len);

                    if(meta != nullptr)
						tgt_meta.append(meta);
					else
						tgt_meta.append(cur->str, cur->len);
				}
				else
				if(cur->chr == '\r' || cur->chr == '\n' || cur->chr == '\t')
				{
					tgt += ' ';
					tgt_meta += ' ';
				}
				else
				{
					tgt += cur->chr;
					tgt_meta += cur->chr;
				}
			}

			if(cur == end)
				break;

			cur = cur->next;
		}
		
		fprintf(file, "<sqlines>%s</sqlines>,<sqlines>%s</sqlines>,<sqlines>%s</sqlines>,<sqlines>%s</sqlines>", src.c_str(), tgt.c_str(), 
			src_meta.c_str(), tgt_meta.c_str());

		fprintf(file, "\n");
		fclose(file);
	}

	first_call = false;
}

// Get meta type for the specified identificator
const char* Stats::GetMetaIdent(Token *name)
{
    if(name == nullptr || name->type != TOKEN_IDENT)
        return nullptr;

	// Check subtypes first as they are more specific
	if(name->data_subtype == TOKEN_DT2_DATETIME)
		return "expr_datetime";
	else
	if(name->data_subtype == TOKEN_DT2_DATETIME_SEC)
		return "expr_datetime_sec";
	else
	if(name->data_subtype == TOKEN_DT2_DATE)
		return "expr_date";
	else
	if(name->data_type == TOKEN_DT_NUMBER)
		return "expr_number";
	else
	if(name->data_type == TOKEN_DT_DATETIME)
		return "expr_datetime";
	
	return "expr_null";
}

// Collect statistics for the specified item
void Stats::Add(std::map<std::string, StatsItem> &map, Token *token, StatsItem *si, bool case_insense)
{
    if(token == nullptr || token->str == nullptr || token->len == 0)
        return;

    std::string source(token->str, token->len);
    Add(map, source, token->t_str, si, case_insense);
}

void Stats::Add(std::map<std::string, StatsItem> &map, Token* start, StatsItem *si, Token* end, bool case_insense)
{
    if(start == nullptr || start->str == nullptr || start->len == 0 || end == nullptr)
        return;

    std::string source(start->str, start->len + start->remain_size - end->remain_size);
    Add(map, source, start->t_str, si, case_insense);
}

void Stats::Add(std::map<std::string, StatsItem> &map, std::string value, const char* target, StatsItem *si,
                    bool case_insense)
{
    if(case_insense)
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);

    // Check if this value already used
	std::map<std::string, StatsItem>::iterator i = map.find(value);

    // Increment the counter or insert new key-value pair
	if(i != map.end())
    	(*i).second.occurrences++;
    else 
	{
		si->occurrences = 1;
		
        if(target != nullptr)
			si->t_value.assign(target);

	    map[value] = *si;
	}
}

// Collect statistics for summary items
void Stats::Add(std::map<std::string, StatsSummaryItem> &map, std::string value, int conv_status)
{
	// Check if this value already used
	std::map<std::string, StatsSummaryItem>::iterator i = map.find(value);

	// Increment the counter or insert new key-value pair
	if(i != map.end())
    	(*i).second.Inc(conv_status);
    else 
	    map[value] = StatsSummaryItem(conv_status);  
}

// Collect statistics for summary items
void Stats::Add(std::map<std::string, StatsSummaryItem> &map, Token *token, int conv_status)
{
    if(token == nullptr || token->str == nullptr || token->len == 0)
        return;

    Add(map, std::string(token->str, token->len), conv_status);
}

// Collect statistics for summary items
void Stats::Add(std::map<std::string, StatsSummaryItem> &map, std::string value, StatsSummaryItem *item, 
	            Token *start, Token *end)
{
    if(item == nullptr)
		return;

	if(item->occurrences == 0)
	{
		item->occurrences++;
		item->conv_undef++;
	}

	item->snippets.push_back(StatsSnippetItem(_source_current_file, start, end));

	// Check if this value already used
	std::map<std::string, StatsSummaryItem>::iterator i = map.find(value);

	// Increment the counter or insert new key-value pair
	if(i != map.end())
    	(*i).second.Inc(item);
    else 
	    map[value] = *item;  
}

// Collect statistics for summary items
void Stats::Add(std::map<std::string, StatsSummaryItem> &map, Token *token, StatsSummaryItem *item, Token *end)
{
    if(token == nullptr || token->str == nullptr || token->len == 0)
        return;

    Add(map, std::string(token->str, token->len), item, token, end);
}

// Collect statistics for detail items
void Stats::Add(std::map<std::string, StatsDetailItem> &map, std::string value, StatsDetailItem *item, Token *start, Token *end)
{
    if(item == nullptr)
		return;

	// Check if this value already used
	std::map<std::string, StatsDetailItem>::iterator i = map.find(value);

	// Each detailed item has the same properties for all occurrences
	if(i == map.end())
	{
		// Create a copy as the item can be added to multiple collections with different snippets
		StatsDetailItem i = *item;

		if(i.count == 0)
			i.count = 1;

		i.snippets.push_back(StatsSnippetItem(_source_current_file, start, end));
		map[value] = i;  
	}
	else
	{
		(*i).second.count++; 
		(*i).second.snippets.push_back(StatsSnippetItem(_source_current_file, start, end));
	}
}

// Collect statistics for detailed items
void Stats::Add(std::map<std::string, StatsDetailItem> &map, Token *token, StatsDetailItem *item, Token *end)
{
    if(token == nullptr || token->str == nullptr || token->len == 0)
        return;

    Add(map, std::string(token->str, token->len), item, token, end);
}

// Collect statistics for detailed items
void Stats::Add(std::map<std::string, StatsDetailItem> &map, Token *start_val, Token *end_val, StatsDetailItem *item, 
	Token *start, Token *end)
{
    if(start_val == nullptr || start_val->str == nullptr || end_val == nullptr || end_val->str == nullptr)
		return;

	std::string value(start_val->str, start_val->len + start_val->remain_size - end_val->remain_size);

	Add(map, value, item, start, end);
}	

// Collect statistics for detailed items
void Stats::Add(std::map<std::string, StatsDetailItem> &map, const char* value, const char* desc, const char* note, const char* link, int conv_status)
{
	// Check if this value already used
	std::map<std::string, StatsDetailItem>::iterator i = map.find(value);

	// Each detailed item has the same properties for all occurrences
	if(i == map.end())
	{
		StatsDetailItem item;
		item.count = 1;
		item.conv_status = conv_status;

        if(desc != nullptr)
			item.desc = desc;
		
        if(note != nullptr)
			item.note = note;

        if(link != nullptr)
			item.link = link;

		map[value] = item;  
	}
	else
		(*i).second.count++;
}

// Collect statistics for detailed items
void Stats::Add(std::map<std::string, StatsDetailItem> &map, Token *start, Token *end, const char* desc, const char* note, const char* link, int conv_status)
{
	Add(map, std::string(start->str, start->len + start->remain_size - end->remain_size).c_str(), desc, note, link, conv_status);
}

// Collect statistics for the specified item
void Stats::Add(std::map<std::string, int> &map, Token *token, bool case_insense)
{
    if(token == nullptr || token->str == nullptr || token->len == 0)
        return;

    Add(map, std::string(token->str, token->len), case_insense);
}

void Stats::Add(std::map<std::string, int> &map, TokenStr *token, bool case_insense)
{
    if(token == nullptr)
        return;

    Add(map, token->str, case_insense);
}

void Stats::Add(std::map<std::string, int> &map, Token *start, Token *end, bool case_insense )
{
    if(start == nullptr || start->str == nullptr || start->len == 0)
        return;

    Add(map, std::string(start->str, start->len + start->remain_size - end->remain_size), case_insense);
}

void Stats::Add(std::map<std::string, int> &map, std::string value, bool case_insense)
{
    if(case_insense)
        std::transform(value.begin(), value.end(), value.begin(), ::toupper);

    // Check if this value already used
	std::map<std::string, int>::iterator i = map.find(value);

    // Increment the counter or insert new key-value pair
	if(i != map.end())
    	(*i).second++;
    else 
	    map[value] = 1;    
}
